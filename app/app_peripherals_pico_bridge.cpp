/**
 * @file app_peripherals_pico_bridge.cpp
 * @brief PICO 4 双手柄双臂 SDK ServoP 遥操作。
 *
 * WebXR 页面通过 adb reverse + localhost WebSocket 发送头显和左右手柄位姿。
 * 本外设应用只维护 Grip 离合、坐标映射和最新 TCP 目标；机器人侧继续使用
 * ControllerManager 的 URKinematics、Ruckig 与安全检查。
 */

#include "peripherals/pico/pico.hpp"
#include "peripherals/pico/singularity_scale.hpp"

#include "sdk/dual_arm_robot.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

using smrcore::peripherals::AlignSingularDirection;
using smrcore::peripherals::AnalyzeJacobian;
using smrcore::peripherals::ClassifySingularityTravel;
using smrcore::peripherals::ConditionNumberToSpeedScale;
using smrcore::peripherals::FilterSingularityRecoveryScale;
using smrcore::peripherals::IsApproachingSingularity;
using smrcore::peripherals::kLeavingSingularityThresholdMultiplier;
using smrcore::peripherals::kSingularityConditionDeltaEpsilon;
using smrcore::peripherals::kSingularityRecoveryFilterTimeConstantSeconds;
using smrcore::peripherals::kSingularitySoftConditionNumber;
using smrcore::peripherals::kSingularityTwistAlignEpsilon;
using smrcore::peripherals::Pico;
using smrcore::peripherals::PicoControllerPose;
using smrcore::peripherals::PicoHeadsetPose;
using smrcore::peripherals::PicoOptions;
using smrcore::peripherals::PicoSample;
using smrcore::peripherals::ResolveTowardFromObservedMotion;
using smrcore::peripherals::SingularityTravel;
using smrcore::peripherals::SingularityTravelToString;

constexpr double kDefaultTranslationScale = 0.80;
constexpr double kDefaultRotationScale = 1.00;
constexpr double kDefaultJ6ThumbstickSpeedRadiansPerSecond = 0.80;
constexpr double kMinimumMappingScale = 0.10;
constexpr double kMaximumMappingScale = 3.00;
// WebXR over adb reverse can hitch past 100ms; freeze+require_release at
// 100ms looks like "PICO moves, arm does not" until the user fully releases
// Grip. 250ms still stops a dropped stream without latching the clutch.
constexpr double kInputStaleSeconds = 0.25;
constexpr double kRobotStateStaleSeconds = 0.25;
constexpr double kGripPressThreshold = 0.90;
constexpr double kGripReleaseThreshold = 0.75;
constexpr double kTriggerBinaryThreshold = 0.50;
constexpr double kJ6ThumbstickDeadzone = 0.18;
constexpr double kJ6ThumbstickAxisSign = -1.0;
constexpr double kPi = 3.14159265358979323846;
constexpr double kHandPositionFilterTimeConstant = 0.0;
constexpr double kHandOrientationFilterTimeConstant = 0.0;
constexpr double kMaxTranslationDeltaMeters = 0.35;
constexpr double kMaxOrientationTargetRateRadiansPerSecond = 3.5;
constexpr double kEulerGimbalLockCosineThreshold = 1e-6;
constexpr auto kServoPeriod = std::chrono::microseconds(1000);
constexpr size_t kServoPBrakeFrames = 100;
constexpr double kServoPStartTranslationMeters = 1e-6;
constexpr double kServoPStartRotationRadians = 1e-5;
constexpr double kSingularityMonitorPeriodSeconds = 0.05;
constexpr double kSingularityMonitorStaleSeconds = 0.25;

std::atomic<bool> g_running{true};

void SignalHandler(int) { g_running.store(false, std::memory_order_release); }

rcore::sdk::Result PublishPicoGripperCommand(rcore::sdk::Robot &robot,
                                             double trigger,
                                             double timestamp_sec)
{
    rcore::SpaceMouseSample sample;
    sample.gripper_command = trigger >= kTriggerBinaryThreshold
                                 ? rcore::GripperCommand::Close
                                 : rcore::GripperCommand::Open;
    sample.timestamp_sec = timestamp_sec;
    return robot.Peripheral().UpdateSpaceMouseSample(sample);
}

rcore::GripperCommand TriggerToGripperCommand(double trigger)
{
    return trigger >= kTriggerBinaryThreshold ? rcore::GripperCommand::Close
                                              : rcore::GripperCommand::Open;
}

struct Options
{
    uint16_t port = 8765;
    bool enable_motion = false;
    bool enable_rotation = true;
    bool head_yaw_clutched = true;
    bool operator_facing_robot = false;
    double translation_scale = kDefaultTranslationScale;
    double rotation_scale = kDefaultRotationScale;
    double j6_thumbstick_speed =
        kDefaultJ6ThumbstickSpeedRadiansPerSecond;
    std::string robot_ip;
};

void PrintUsage(const char *program)
{
    std::cout << "用法: " << program
              << " [--enable-motion] [--translation-only] [--room-fixed]"
                 " [--operator-frame same-side|face-to-face] [--port 8765]"
                 " [--motion-scale 1.0] [--translation-scale 0.8]"
                 " [--rotation-scale 1.0] [--j6-speed 0.8]"
                 " [--robot-ip ADDRESS]\n"
              << "--enable-motion 是必需的实机安全确认；控制链仅为 SDK "
                 "ServoP + J6 关节透传。按住 Grip 后推对应手柄推杆"
                 "上下控制本臂 J6。\n";
}

bool ParseOperatorFrame(const std::string &text, bool &operator_facing_robot)
{
    if (text == "same-side")
    {
        operator_facing_robot = false;
        return true;
    }
    if (text == "face-to-face")
    {
        operator_facing_robot = true;
        return true;
    }
    return false;
}

bool ParsePort(const std::string &text, uint16_t &port)
{
    try
    {
        const unsigned long value = std::stoul(text);
        if (value == 0 || value > 65535)
        {
            return false;
        }
        port = static_cast<uint16_t>(value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ParseMappingScale(const std::string &text, double &scale)
{
    try
    {
        size_t parsed = 0;
        const double value = std::stod(text, &parsed);
        if (parsed != text.size() || !std::isfinite(value) ||
            value < kMinimumMappingScale || value > kMaximumMappingScale)
        {
            return false;
        }
        scale = value;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ParseOptions(int argc, char **argv, Options &options)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--enable-motion")
        {
            options.enable_motion = true;
        }
        else if (argument == "--translation-only")
        {
            options.enable_rotation = false;
        }
        else if (argument == "--room-fixed")
        {
            options.head_yaw_clutched = false;
        }
        else if (argument == "--operator-frame" && i + 1 < argc)
        {
            if (!ParseOperatorFrame(argv[++i], options.operator_facing_robot))
            {
                return false;
            }
        }
        else if (argument == "--port" && i + 1 < argc)
        {
            if (!ParsePort(argv[++i], options.port))
            {
                return false;
            }
        }
        else if (argument == "--motion-scale" && i + 1 < argc)
        {
            double scale = 0.0;
            if (!ParseMappingScale(argv[++i], scale))
            {
                return false;
            }
            options.translation_scale = scale;
            options.rotation_scale = scale;
        }
        else if (argument == "--translation-scale" && i + 1 < argc)
        {
            if (!ParseMappingScale(argv[++i], options.translation_scale))
            {
                return false;
            }
        }
        else if (argument == "--rotation-scale" && i + 1 < argc)
        {
            if (!ParseMappingScale(argv[++i], options.rotation_scale))
            {
                return false;
            }
        }
        else if (argument == "--j6-speed" && i + 1 < argc)
        {
            if (!ParseMappingScale(argv[++i], options.j6_thumbstick_speed))
            {
                return false;
            }
        }
        else if (argument == "--robot-ip" && i + 1 < argc)
        {
            options.robot_ip = argv[++i];
        }
        else if (argument == "--help" || argument == "-h")
        {
            PrintUsage(argv[0]);
            std::exit(0);
        }
        else
        {
            return false;
        }
    }
    return true;
}

Eigen::Matrix3d EulerZyxToRotation(const Eigen::Vector3d &rpy)
{
    return (Eigen::AngleAxisd(rpy[2], Eigen::Vector3d::UnitZ()) *
            Eigen::AngleAxisd(rpy[1], Eigen::Vector3d::UnitY()) *
            Eigen::AngleAxisd(rpy[0], Eigen::Vector3d::UnitX()))
        .toRotationMatrix();
}

double UnwrapNear(double angle, double reference)
{
    while (angle - reference > kPi)
    {
        angle -= 2.0 * kPi;
    }
    while (angle - reference < -kPi)
    {
        angle += 2.0 * kPi;
    }
    return angle;
}

Eigen::Vector3d UnwrapRpyNear(Eigen::Vector3d rpy,
                              const Eigen::Vector3d &reference)
{
    for (Eigen::Index axis = 0; axis < 3; ++axis)
    {
        rpy[axis] = UnwrapNear(rpy[axis], reference[axis]);
    }
    return rpy;
}

Eigen::Vector3d RotationToContinuousEulerZyx(const Eigen::Matrix3d &rotation,
                                             const Eigen::Vector3d &reference)
{
    const double sine_pitch = std::clamp(-rotation(2, 0), -1.0, 1.0);
    const double pitch = std::asin(sine_pitch);
    const double cosine_pitch = std::cos(pitch);
    if (std::abs(cosine_pitch) <= kEulerGimbalLockCosineThreshold)
    {
        // At pitch = +/-pi/2 only a roll/yaw combination is observable.
        // Preserve the previous roll and solve the coupled yaw instead of
        // resetting roll to zero, which would create a ServoP branch jump.
        const double roll = reference[0];
        const double coupled_yaw = std::atan2(-rotation(0, 1), rotation(1, 1));
        const double yaw =
            sine_pitch >= 0.0 ? roll + coupled_yaw : coupled_yaw - roll;
        return UnwrapRpyNear({roll, std::copysign(kPi / 2.0, sine_pitch), yaw},
                             reference);
    }

    const Eigen::Vector3d primary{
        std::atan2(rotation(2, 1), rotation(2, 2)),
        pitch,
        std::atan2(rotation(1, 0), rotation(0, 0)),
    };
    // ZYX has two equivalent branches away from gimbal lock. Select the one
    // nearest the last command so crossing +/-90 deg pitch stays continuous.
    const Eigen::Vector3d alternate{
        primary[0] + kPi,
        kPi - primary[1],
        primary[2] + kPi,
    };
    const Eigen::Vector3d primary_near = UnwrapRpyNear(primary, reference);
    const Eigen::Vector3d alternate_near = UnwrapRpyNear(alternate, reference);
    return (primary_near - reference).squaredNorm() <=
                   (alternate_near - reference).squaredNorm()
               ? primary_near
               : alternate_near;
}

Eigen::Isometry3d MakeIsometry(const Eigen::Vector3d &xyz,
                               const Eigen::Vector3d &rpy)
{
    Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
    transform.translation() = xyz;
    transform.linear() = EulerZyxToRotation(rpy);
    return transform;
}

Eigen::Isometry3d MakeIsometry(const rcore::FramePose &pose)
{
    return MakeIsometry(Eigen::Vector3d(pose.xyz[0], pose.xyz[1], pose.xyz[2]),
                        Eigen::Vector3d(pose.rpy[0], pose.rpy[1], pose.rpy[2]));
}

Eigen::Isometry3d MakeIsometry(const rcore::sdk::Pose &pose)
{
    return MakeIsometry(
        Eigen::Vector3d(pose.tvec()[0], pose.tvec()[1], pose.tvec()[2]),
        Eigen::Vector3d(pose.rvec()[0], pose.rvec()[1], pose.rvec()[2]));
}

bool IsNearIdentity(const Eigen::Isometry3d &transform)
{
    return transform.translation().cwiseAbs().maxCoeff() <= 1e-6 &&
           (transform.linear() - Eigen::Matrix3d::Identity()).norm() < 1e-3;
}

bool ParseUrdfJointOrigin(const std::string &xml, const std::string &joint_name,
                          Eigen::Isometry3d &transform)
{
    const size_t joint = xml.find("name=\"" + joint_name + "\"");
    const size_t joint_end = joint == std::string::npos
                                 ? std::string::npos
                                 : xml.find("</joint>", joint);
    if (joint == std::string::npos || joint_end == std::string::npos)
    {
        return false;
    }
    const std::string block = xml.substr(joint, joint_end - joint);
    const size_t origin = block.find("<origin");
    const size_t origin_end = origin == std::string::npos
                                  ? std::string::npos
                                  : block.find('>', origin);
    if (origin == std::string::npos || origin_end == std::string::npos)
    {
        return false;
    }
    const std::string tag = block.substr(origin, origin_end - origin);

    auto read_vector = [&](const char *name, Eigen::Vector3d &value)
    {
        const std::string key = std::string(name) + "=\"";
        const size_t found = tag.find(key);
        if (found == std::string::npos)
        {
            return false;
        }
        const size_t begin = found + key.size();
        const size_t end = tag.find('"', begin);
        if (end == std::string::npos)
        {
            return false;
        }
        std::istringstream stream(tag.substr(begin, end - begin));
        return static_cast<bool>(stream >> value[0] >> value[1] >> value[2]);
    };

    Eigen::Vector3d xyz = Eigen::Vector3d::Zero();
    Eigen::Vector3d rpy = Eigen::Vector3d::Zero();
    if (!read_vector("xyz", xyz))
    {
        return false;
    }
    read_vector("rpy", rpy);
    transform = MakeIsometry(xyz, rpy);
    return true;
}

struct UrdfMounts
{
    bool valid = false;
    Eigen::Isometry3d left = Eigen::Isometry3d::Identity();
    Eigen::Isometry3d right = Eigen::Isometry3d::Identity();
    std::string path;
};

UrdfMounts LoadB3UrdfMounts()
{
    const std::vector<std::string> paths = {
        "data/model/SMR-B3-000-URDF/urdf/"
        "SMR-B3-000-URDF_V260630.zip.urdf",
        "build_Release_x86/install/data/model/SMR-B3-000-URDF/urdf/"
        "SMR-B3-000-URDF_V260630.zip.urdf",
        "../data/model/SMR-B3-000-URDF/urdf/"
        "SMR-B3-000-URDF_V260630.zip.urdf",
        "../../data/model/SMR-B3-000-URDF/urdf/"
        "SMR-B3-000-URDF_V260630.zip.urdf",
    };
    for (const auto &path : paths)
    {
        std::ifstream input(path);
        if (!input)
        {
            continue;
        }
        std::ostringstream buffer;
        buffer << input.rdbuf();
        UrdfMounts result;
        if (ParseUrdfJointOrigin(buffer.str(), "left_base_joint",
                                 result.left) &&
            ParseUrdfJointOrigin(buffer.str(), "right_base_joint",
                                 result.right))
        {
            // V260630 整机 URDF 的 base_joint origin = mount×Trans(z=0.1245)，
            // 旋转尚未含 MJCF L/R_base_Link 的局部 Rz(π)。runtime reference
            // left/right_b3_arm_base 对齐 arm-base，必须后乘 Rz(π)。
            Eigen::Isometry3d base_link_rz_pi = Eigen::Isometry3d::Identity();
            base_link_rz_pi.linear() =
                Eigen::AngleAxisd(kPi, Eigen::Vector3d::UnitZ())
                    .toRotationMatrix();
            result.left = result.left * base_link_rz_pi;
            result.right = result.right * base_link_rz_pi;
            result.valid = true;
            result.path = path;
            return result;
        }
    }
    return {};
}

bool ResolveParentChain(rcore::sdk::Robot &robot,
                        const std::string &reference_frame,
                        Eigen::Isometry3d &root_from_reference,
                        bool &ends_at_world, std::string &error)
{
    root_from_reference = Eigen::Isometry3d::Identity();
    ends_at_world = false;
    if (reference_frame.empty())
    {
        error = "reference_frame 为空";
        return false;
    }
    if (reference_frame == "base" || reference_frame == "world")
    {
        ends_at_world = reference_frame == "world";
        return true;
    }

    std::unordered_map<std::string, rcore::WorkpieceFrameInfo> frames;
    for (const auto &frame : robot.Frames().GetWorkpieceFramesInfo())
    {
        frames.emplace(frame.name, frame);
    }

    Eigen::Isometry3d accumulated = Eigen::Isometry3d::Identity();
    std::string current = reference_frame;
    for (int depth = 0; depth < 64; ++depth)
    {
        if (current == "base" || current == "world")
        {
            root_from_reference = accumulated;
            ends_at_world = current == "world";
            return true;
        }
        const auto found = frames.find(current);
        if (found == frames.end())
        {
            error = "参考系不在 Frame 树中: " + current;
            return false;
        }
        accumulated = MakeIsometry(found->second.pose) * accumulated;
        current = found->second.parent;
        if (current.empty())
        {
            error = "参考系父链断开: " + found->second.name;
            return false;
        }
    }
    error = "参考系父链过深: " + reference_frame;
    return false;
}

struct ArmReference
{
    bool valid = false;
    std::string name;
    Eigen::Isometry3d world_from_reference = Eigen::Isometry3d::Identity();
    Eigen::Matrix3d base_from_reference = Eigen::Matrix3d::Identity();
};

bool ResolveArmReference(rcore::sdk::Robot &robot,
                         const rcore::RobotState &state,
                         const Eigen::Isometry3d *urdf_mount,
                         ArmReference &result, std::string &error)
{
    result = {};
    const std::string &reference = state.reference_frame;
    if (reference == "world")
    {
        rcore::sdk::Pose base_pose;
        auto base_result = robot.Frames().GetBasePose(base_pose);
        if (!base_result.IsSuccess())
        {
            error = "world reference 下 GetBasePose 失败";
            return false;
        }
        result.base_from_reference =
            MakeIsometry(base_pose).linear().transpose();
        result.valid = true;
        result.name = reference;
        return true;
    }

    Eigen::Isometry3d root_from_reference = Eigen::Isometry3d::Identity();
    bool ends_at_world = false;
    if (!ResolveParentChain(robot, reference, root_from_reference,
                            ends_at_world, error))
    {
        return false;
    }
    if (ends_at_world)
    {
        rcore::sdk::Pose base_pose;
        auto base_result = robot.Frames().GetBasePose(base_pose);
        if (!base_result.IsSuccess())
        {
            error = "world 父链下 GetBasePose 失败";
            return false;
        }
        const Eigen::Matrix3d base_from_world =
            MakeIsometry(base_pose).linear().transpose();
        result.name = reference;
        result.world_from_reference = root_from_reference;
        result.base_from_reference =
            base_from_world * root_from_reference.linear();
        result.valid = result.world_from_reference.matrix().allFinite() &&
                       result.base_from_reference.allFinite();
        return result.valid;
    }

    rcore::sdk::Pose base_pose;
    auto base_result = robot.Frames().GetBasePose(base_pose);
    Eigen::Isometry3d world_from_base = Eigen::Isometry3d::Identity();
    if (base_result.IsSuccess())
    {
        world_from_base = MakeIsometry(base_pose);
    }
    // 与 app_sdk_teleop_dual 对齐：实机默认 reference 常为 "base"；
    // GetBasePose≈I 时 controller 不知道 B3 斜装，必须用 URDF world→arm-base。
    const bool b3_physical_arm_reference = reference == "base" ||
                                           reference == "left_b3_arm_base" ||
                                           reference == "right_b3_arm_base";
    if (urdf_mount != nullptr && b3_physical_arm_reference &&
        (!base_result.IsSuccess() || IsNearIdentity(world_from_base)))
    {
        result.name = reference;
        result.world_from_reference = *urdf_mount;
        result.base_from_reference = root_from_reference.linear();
        result.valid = result.world_from_reference.matrix().allFinite() &&
                       result.base_from_reference.allFinite();
        return result.valid;
    }
    if (!base_result.IsSuccess())
    {
        error = "GetBasePose 失败，且没有可用的 B3 安装变换";
        return false;
    }
    result.name = reference;
    result.world_from_reference = world_from_base * root_from_reference;
    result.base_from_reference = root_from_reference.linear();
    result.valid = result.world_from_reference.matrix().allFinite() &&
                   result.base_from_reference.allFinite();
    if (!result.valid)
    {
        error = "world_from_reference 含非有限值";
    }
    return result.valid;
}

bool ResolveDualReferences(rcore::sdk::DualArmRobot &arms,
                           const UrdfMounts &mounts, ArmReference &left,
                           ArmReference &right, std::string &error)
{
    const auto left_state = arms.left_robot().GetState();
    const auto right_state = arms.right_robot().GetState();
    std::string left_error;
    std::string right_error;
    const bool left_ok = ResolveArmReference(
        arms.left_robot(), left_state, mounts.valid ? &mounts.left : nullptr,
        left, left_error);
    const bool right_ok = ResolveArmReference(
        arms.right_robot(), right_state, mounts.valid ? &mounts.right : nullptr,
        right, right_error);
    if (!left_ok || !right_ok)
    {
        error = "L: " + left_error + "; R: " + right_error;
        return false;
    }
    return true;
}

Eigen::Matrix3d WebXrFromGrip(const PicoControllerPose &pose)
{
    const Eigen::Quaterniond quaternion(
        pose.orientation_xyzw[3], pose.orientation_xyzw[0],
        pose.orientation_xyzw[1], pose.orientation_xyzw[2]);
    if (quaternion.norm() < 1e-9)
    {
        return Eigen::Matrix3d::Identity();
    }
    return quaternion.normalized().toRotationMatrix();
}

Eigen::Vector3d WebXrPosition(const PicoControllerPose &pose)
{
    return {pose.position[0], pose.position[1], pose.position[2]};
}

Eigen::Matrix3d RobotWorldFromWebXr()
{
    Eigen::Matrix3d transform;
    // WebXR: +X right, +Y up, -Z forward.
    // Robot world: +X forward, +Y left, +Z up.
    transform << 0.0, 0.0, -1.0, -1.0, 0.0, 0.0, 0.0, 1.0, 0.0;
    return transform;
}

Eigen::Matrix3d
ApplyOperatorFrame(const Eigen::Matrix3d &robot_world_from_webxr,
                   bool operator_facing_robot)
{
    if (!operator_facing_robot)
    {
        return robot_world_from_webxr;
    }
    // 面对面时操作者的前/左分别对应机器人的后/右，竖直轴保持一致。
    const Eigen::Matrix3d face_to_face_rotation =
        Eigen::AngleAxisd(kPi, Eigen::Vector3d::UnitZ()).toRotationMatrix();
    return face_to_face_rotation * robot_world_from_webxr;
}

bool RobotWorldFromHeadYaw(const PicoHeadsetPose &headset,
                           Eigen::Matrix3d &robot_world_from_webxr,
                           double &head_yaw)
{
    if (!headset.present || !headset.tracked)
    {
        return false;
    }
    const Eigen::Quaterniond quaternion(
        headset.orientation_xyzw[3], headset.orientation_xyzw[0],
        headset.orientation_xyzw[1], headset.orientation_xyzw[2]);
    if (!std::isfinite(quaternion.norm()) || quaternion.norm() < 1e-9)
    {
        return false;
    }

    Eigen::Vector3d forward_webxr =
        quaternion.normalized().toRotationMatrix() * -Eigen::Vector3d::UnitZ();
    forward_webxr.y() = 0.0;
    if (!forward_webxr.allFinite() || forward_webxr.norm() < 1e-3)
    {
        return false;
    }
    forward_webxr.normalize();
    const Eigen::Vector3d up_webxr = Eigen::Vector3d::UnitY();
    const Eigen::Vector3d left_webxr =
        up_webxr.cross(forward_webxr).normalized();

    Eigen::Matrix3d webxr_from_head_yaw;
    webxr_from_head_yaw.col(0) = forward_webxr;
    webxr_from_head_yaw.col(1) = left_webxr;
    webxr_from_head_yaw.col(2) = up_webxr;
    robot_world_from_webxr = webxr_from_head_yaw.transpose();
    head_yaw = std::atan2(forward_webxr.x(), -forward_webxr.z());
    return robot_world_from_webxr.allFinite() &&
           std::abs(robot_world_from_webxr.determinant() - 1.0) < 1e-6 &&
           std::isfinite(head_yaw);
}

double SecondsSince(const std::chrono::steady_clock::time_point &time)
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                         time)
        .count();
}

double SecondsSince(double timestamp_sec)
{
    const double now_sec =
        std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();
    return now_sec - timestamp_sec;
}

bool TakePicoSample(Pico &pico, PicoSample &sample)
{
    auto next = pico.GetSample();
    if (!next)
    {
        return false;
    }
    sample = std::move(*next);
    return true;
}

struct HandServoState
{
    bool active = false;
    bool require_release = false;
    bool target_valid = false;
    Eigen::Vector3d hand_start_position = Eigen::Vector3d::Zero();
    Eigen::Vector3d filtered_hand_position = Eigen::Vector3d::Zero();
    Eigen::Quaterniond filtered_hand_orientation =
        Eigen::Quaterniond::Identity();
    Eigen::Matrix3d robot_world_from_webxr_clutch = RobotWorldFromWebXr();
    double clutch_head_yaw = 0.0;
    Eigen::Isometry3d tcp_start_world = Eigen::Isometry3d::Identity();
    Eigen::Isometry3d target_tcp_reference = Eigen::Isometry3d::Identity();
    Eigen::Vector3d target_rpy = Eigen::Vector3d::Zero();
    bool servop_stream_started = false;
    size_t servop_brake_frames_remaining = 0;
    bool joint6_active = false;
    bool joint6_target_valid = false;
    rcore::sdk::JointPositions joint_target{};
    double joint6_velocity = 0.0;
    size_t joint6_brake_frames_remaining = 0;
    std::chrono::steady_clock::time_point last_joint6_update{};
    uint64_t command_frames = 0;
    double translation_step = 0.0;
    double rotation_step = 0.0;
    Eigen::Vector3d last_world_delta = Eigen::Vector3d::Zero();
    std::chrono::steady_clock::time_point last_xr_update{};
    double singularity_scale = 1.0;
    int64_t singularity_scale_updated_ns = 0;
    SingularityTravel singularity_travel = SingularityTravel::Nominal;
};

struct SingularitySlowdownStatus
{
    bool fresh = false;
    bool toward_valid = false;
    double condition_number = std::numeric_limits<double>::infinity();
    Eigen::Matrix<double, 6, 1> toward_singularity =
        Eigen::Matrix<double, 6, 1>::Zero();
};

int64_t SteadyNowNanoseconds()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

class SingularitySlowdownMonitor
{
public:
    explicit SingularitySlowdownMonitor(rcore::sdk::Robot &robot)
        : robot_(robot)
    {
    }

    ~SingularitySlowdownMonitor() { Stop(); }

    SingularitySlowdownMonitor(const SingularitySlowdownMonitor &) = delete;
    SingularitySlowdownMonitor &
    operator=(const SingularitySlowdownMonitor &) = delete;

    void Start()
    {
        if (running_.exchange(true, std::memory_order_acq_rel))
        {
            return;
        }
        thread_ = std::thread(&SingularitySlowdownMonitor::Run, this);
    }

    void Stop()
    {
        if (!running_.exchange(false, std::memory_order_acq_rel))
        {
            return;
        }
        if (thread_.joinable())
        {
            thread_.join();
        }
    }

    bool WaitForInitialSample(std::chrono::milliseconds timeout) const
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (g_running.load(std::memory_order_acquire) &&
               std::chrono::steady_clock::now() < deadline)
        {
            if (Snapshot().fresh)
            {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return Snapshot().fresh;
    }

    SingularitySlowdownStatus Snapshot() const
    {
        SingularitySlowdownStatus result;
        int64_t updated_at_ns = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            updated_at_ns = updated_at_ns_;
            result.condition_number = condition_number_;
            result.toward_singularity = toward_singularity_;
            result.toward_valid = toward_valid_;
        }
        const double age =
            static_cast<double>(SteadyNowNanoseconds() - updated_at_ns) / 1e9;
        result.fresh = updated_at_ns > 0 && std::isfinite(age) && age >= 0.0 &&
                       age <= kSingularityMonitorStaleSeconds;
        return result;
    }

private:
    static Eigen::Matrix<double, 6, 1>
    BaseTwistFromState(const rcore::RobotState &state)
    {
        Eigen::Matrix<double, 6, 1> twist;
        for (Eigen::Index axis = 0; axis < 6; ++axis)
        {
            twist[axis] = state.cartesian_velocity[static_cast<size_t>(axis)];
        }
        return twist;
    }

    void Run()
    {
        auto next_sample = std::chrono::steady_clock::now();
        bool have_previous = false;
        bool previous_toward_valid = false;
        double previous_condition = std::numeric_limits<double>::infinity();
        Eigen::Matrix<double, 6, 1> previous_toward =
            Eigen::Matrix<double, 6, 1>::Zero();
        while (running_.load(std::memory_order_acquire))
        {
            rcore::sdk::Jacobian sdk_jacobian;
            auto result = robot_.Motion().GetJacobian(sdk_jacobian);
            if (result.IsSuccess())
            {
                Eigen::Matrix<double, 6, 6> jacobian;
                for (Eigen::Index row = 0; row < 6; ++row)
                {
                    for (Eigen::Index column = 0; column < 6; ++column)
                    {
                        jacobian(row, column) =
                            sdk_jacobian(static_cast<size_t>(row),
                                         static_cast<size_t>(column));
                    }
                }

                const auto analysis = AnalyzeJacobian(jacobian);
                if (analysis.valid)
                {
                    Eigen::Matrix<double, 6, 1> toward =
                        analysis.least_singular_direction;
                    bool toward_valid = false;
                    const auto robot_state = robot_.GetState();
                    const Eigen::Matrix<double, 6, 1> base_twist =
                        BaseTwistFromState(robot_state);
                    const bool twist_usable =
                        base_twist.allFinite() &&
                        base_twist.norm() > kSingularityTwistAlignEpsilon;
                    if (have_previous && twist_usable &&
                        std::isfinite(previous_condition) &&
                        std::abs(analysis.condition_number -
                                 previous_condition) >
                            kSingularityConditionDeltaEpsilon)
                    {
                        toward = ResolveTowardFromObservedMotion(
                            toward, base_twist, analysis.condition_number,
                            previous_condition);
                        toward_valid = true;
                    }
                    else if (have_previous && previous_toward_valid)
                    {
                        toward =
                            AlignSingularDirection(toward, previous_toward);
                        toward_valid = true;
                    }

                    previous_condition = analysis.condition_number;
                    previous_toward = toward;
                    previous_toward_valid = toward_valid;
                    have_previous = true;

                    const int64_t now_ns = SteadyNowNanoseconds();
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        condition_number_ = analysis.condition_number;
                        toward_singularity_ = toward;
                        toward_valid_ = toward_valid;
                        updated_at_ns_ = now_ns;
                    }
                }
            }

            next_sample +=
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<double>(
                        kSingularityMonitorPeriodSeconds));
            const auto now = std::chrono::steady_clock::now();
            if (next_sample < now)
            {
                next_sample = now;
            }
            std::this_thread::sleep_until(next_sample);
        }
    }

    rcore::sdk::Robot &robot_;
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
    double condition_number_{std::numeric_limits<double>::infinity()};
    Eigen::Matrix<double, 6, 1> toward_singularity_{
        Eigen::Matrix<double, 6, 1>::Zero()};
    bool toward_valid_{false};
    int64_t updated_at_ns_{0};
    std::thread thread_;
};

double FilterAlpha(double dt, double time_constant)
{
    if (!std::isfinite(dt) || dt <= 0.0 || time_constant <= 0.0)
    {
        return 1.0;
    }
    return 1.0 - std::exp(-dt / time_constant);
}

bool GetTcpInReference(const rcore::RobotState &robot_state,
                       Eigen::Isometry3d &tcp_in_reference)
{
    if (!robot_state.cartesian_valid)
    {
        return false;
    }
    const Eigen::Vector3d position(robot_state.cartesian_position[0],
                                   robot_state.cartesian_position[1],
                                   robot_state.cartesian_position[2]);
    const Eigen::Vector3d rpy(robot_state.cartesian_orientation[0],
                              robot_state.cartesian_orientation[1],
                              robot_state.cartesian_orientation[2]);
    tcp_in_reference = MakeIsometry(position, rpy);
    return tcp_in_reference.matrix().allFinite();
}

bool SetTargetFromRobotState(const rcore::RobotState &robot_state,
                             HandServoState &state)
{
    Eigen::Isometry3d current_tcp = Eigen::Isometry3d::Identity();
    if (!GetTcpInReference(robot_state, current_tcp))
    {
        state.target_valid = false;
        return false;
    }
    state.target_tcp_reference = current_tcp;
    state.target_rpy = {
        robot_state.cartesian_orientation[0],
        robot_state.cartesian_orientation[1],
        robot_state.cartesian_orientation[2],
    };
    state.target_valid = state.target_rpy.allFinite();
    return state.target_valid;
}

bool SetJointTargetFromRobotState(const rcore::RobotState &robot_state,
                                  HandServoState &state)
{
    for (size_t joint = 0; joint < 6; ++joint)
    {
        if (!std::isfinite(robot_state.positions[joint]))
        {
            state.joint6_target_valid = false;
            return false;
        }
        state.joint_target[joint] = robot_state.positions[joint];
    }
    state.joint6_target_valid = true;
    return true;
}

void FreezeHand(const rcore::RobotState &robot_state, HandServoState &state,
                bool require_release)
{
    const bool should_servop_brake =
        state.active && state.servop_stream_started && !require_release;
    const bool should_joint6_brake = state.joint6_active && !require_release;
    const bool joint_target_valid = SetJointTargetFromRobotState(robot_state,
                                                                  state);
    state.active = false;
    state.joint6_active = false;
    state.require_release = state.require_release || require_release;
    state.servop_stream_started = false;
    state.joint6_velocity = 0.0;
    state.last_joint6_update = {};
    state.translation_step = 0.0;
    state.rotation_step = 0.0;
    state.singularity_scale = 1.0;
    state.singularity_scale_updated_ns = 0;
    state.singularity_travel = SingularityTravel::Nominal;
    state.servop_brake_frames_remaining =
        should_servop_brake ? kServoPBrakeFrames : 0;
    state.joint6_brake_frames_remaining =
        should_joint6_brake && joint_target_valid ? kServoPBrakeFrames : 0;
    (void)SetTargetFromRobotState(robot_state, state);
}

double NormalizeJ6Thumbstick(double axis)
{
    if (!std::isfinite(axis))
    {
        return 0.0;
    }
    axis = std::clamp(axis, -1.0, 1.0);
    const double magnitude = std::abs(axis);
    if (magnitude <= kJ6ThumbstickDeadzone)
    {
        return 0.0;
    }
    const double remapped =
        (magnitude - kJ6ThumbstickDeadzone) /
        (1.0 - kJ6ThumbstickDeadzone);
    return std::copysign(remapped, kJ6ThumbstickAxisSign * axis);
}

bool ReadJointPositions(const rcore::RobotState &robot_state,
                        rcore::sdk::JointPositions &positions)
{
    for (size_t joint = 0; joint < 6; ++joint)
    {
        if (!std::isfinite(robot_state.positions[joint]))
        {
            return false;
        }
        positions[joint] = robot_state.positions[joint];
    }
    return true;
}

bool JointsNearStill(const rcore::RobotState &robot_state)
{
    constexpr double kDisableVelocityThreshold = 0.01;
    for (size_t joint = 0; joint < 6; ++joint)
    {
        if (!std::isfinite(robot_state.velocities[joint]) ||
            std::abs(robot_state.velocities[joint]) > kDisableVelocityThreshold)
        {
            return false;
        }
    }
    return true;
}

void PrintDisableStatus(const char *arm, rcore::sdk::Result &result,
                        const rcore::MotorStatus &status)
{
    std::cout << arm << " Disable rpc=" << (result.IsSuccess() ? "ok" : "fail")
              << "(" << result.GetErrorCode() << ") " << result.GetErrorMsg()
              << " enabled=" << (status.enabled ? "true" : "false")
              << " operational=" << (status.operational ? "true" : "false")
              << std::endl;
}

// Cia402Motor::Disable() refuses when any axis exceeds 0.01 rad/s. The
// tracking arm is often still catching the last ServoP when Ctrl+C arrives,
// while the idle arm is already still — only the idle arm would drop enable.
void HoldThenDisable(rcore::sdk::Robot &robot, const char *arm)
{
    rcore::sdk::JointPositions hold;
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto state = robot.GetState();
        if (state.errors.GetLatchedErrorCount() > 0)
        {
            break;
        }
        if (!ReadJointPositions(state, hold))
        {
            break;
        }
        robot.Motion().ServoJ(hold);
        if (JointsNearStill(state))
        {
            break;
        }
        std::this_thread::sleep_for(kServoPeriod);
    }

    for (int attempt = 0; attempt < 5; ++attempt)
    {
        auto result = robot.Disable();
        const auto status = robot.Control().GetMotorStatus();
        PrintDisableStatus(arm, result, status);
        if (result.IsSuccess() && !status.enabled)
        {
            return;
        }
        const auto state = robot.GetState();
        if (ReadJointPositions(state, hold))
        {
            robot.Motion().ServoJ(hold);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cerr << arm << " 退出时仍使能，请在硬件侧确认 Disable。" << std::endl;
}

bool EnsureSdkCommandSource(rcore::sdk::Robot &robot, const char *arm,
                            std::string &error)
{
    if (robot.Control().GetCommandSource() == rcore::sdk::CommandSource::Sdk)
    {
        return true;
    }
    auto result =
        robot.Control().SetCommandSource(rcore::sdk::CommandSource::Sdk);
    if (!result.IsSuccess())
    {
        error = std::string(arm) + " 获取 SDK 命令源失败: [" +
                std::to_string(result.GetErrorCode()) + "] " +
                result.GetErrorMsg();
        return false;
    }

    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (robot.Control().GetCommandSource() ==
            rcore::sdk::CommandSource::Sdk)
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    error = std::string(arm) + " SDK 命令源状态 1s 内未同步";
    return false;
}

class RobotStateFreshnessWatchdog
{
public:
    bool Observe(const rcore::RobotState &robot_state,
                 std::chrono::steady_clock::time_point observed_at)
    {
        if (!std::isfinite(robot_state.timestamp))
        {
            return false;
        }
        if (!seen_ || robot_state.timestamp != last_timestamp_)
        {
            seen_ = true;
            last_timestamp_ = robot_state.timestamp;
            last_change_at_ = observed_at;
        }
        return seen_ &&
               std::chrono::duration<double>(observed_at - last_change_at_)
                       .count() <= kRobotStateStaleSeconds;
    }

private:
    bool seen_ = false;
    double last_timestamp_ = 0.0;
    std::chrono::steady_clock::time_point last_change_at_{};
};

void PrintLatchedErrors(const char *arm, const rcore::RobotState &robot_state)
{
    const auto &errors = robot_state.errors.GetLatchedErrors();
    for (size_t index = 0; index < robot_state.errors.GetLatchedErrorCount();
         ++index)
    {
        std::cerr << " " << arm << "[code=" << errors[index].code
                  << ",level=" << static_cast<int>(errors[index].level) << "]";
    }
}

double OrientationErrorDegrees(const rcore::RobotState &robot_state,
                               const HandServoState &state)
{
    if (!robot_state.cartesian_valid)
    {
        return -1.0;
    }
    const Eigen::Matrix3d actual_rotation =
        EulerZyxToRotation({robot_state.cartesian_orientation[0],
                            robot_state.cartesian_orientation[1],
                            robot_state.cartesian_orientation[2]});
    const Eigen::AngleAxisd error(actual_rotation.transpose() *
                                  state.target_tcp_reference.linear());
    return std::isfinite(error.angle()) ? error.angle() * 180.0 / kPi : -1.0;
}

double PositionErrorMillimeters(const rcore::RobotState &robot_state,
                                const HandServoState &state)
{
    if (!robot_state.cartesian_valid)
    {
        return -1.0;
    }
    const Eigen::Vector3d actual(robot_state.cartesian_position[0],
                                 robot_state.cartesian_position[1],
                                 robot_state.cartesian_position[2]);
    const double error =
        (state.target_tcp_reference.translation() - actual).norm() * 1000.0;
    return std::isfinite(error) ? error : -1.0;
}

bool SendSdkServoPTick(rcore::sdk::Robot &robot, HandServoState &state)
{
    const bool joint6_braking = state.joint6_brake_frames_remaining > 0;
    if (state.joint6_active || joint6_braking)
    {
        if (!state.joint6_target_valid)
        {
            return false;
        }

        if (state.joint6_active)
        {
            const auto now = std::chrono::steady_clock::now();
            double dt = std::chrono::duration<double>(
                            now - state.last_joint6_update)
                            .count();
            if (!std::isfinite(dt) || dt <= 0.0 || dt > kInputStaleSeconds)
            {
                dt = 0.0;
            }
            state.joint_target[5] += state.joint6_velocity * dt;
            state.last_joint6_update = now;
            if (!std::isfinite(state.joint_target[5]))
            {
                state.joint6_active = false;
                state.joint6_target_valid = false;
                return false;
            }
        }

        rcore::sdk::JointVelocities velocities{};
        if (state.joint6_active)
        {
            velocities[5] = state.joint6_velocity;
        }
        robot.Motion().JointPassthrough(state.joint_target, velocities);
        if (joint6_braking)
        {
            --state.joint6_brake_frames_remaining;
        }
        return true;
    }

    const bool tracking = state.active && state.servop_stream_started;
    const bool braking = state.servop_brake_frames_remaining > 0;
    if ((!tracking && !braking) || !state.target_valid ||
        !state.target_tcp_reference.matrix().allFinite() ||
        !state.target_rpy.allFinite())
    {
        return false;
    }

    rcore::sdk::Pose pose;
    for (Eigen::Index axis = 0; axis < 3; ++axis)
    {
        pose.tvec()[axis] = state.target_tcp_reference.translation()[axis];
        pose.rvec()[axis] = state.target_rpy[axis];
    }
    robot.Motion().ServoP(pose);
    if (braking)
    {
        --state.servop_brake_frames_remaining;
    }
    return true;
}

void PrintArmDiagnostics(const char *name, const PicoControllerPose &pose,
                         const rcore::RobotState &robot_state,
                         const HandServoState &hand,
                         const ArmReference &reference,
                         const SingularitySlowdownStatus &singularity)
{
    const Eigen::Isometry3d actual_reference = [&]()
    {
        Eigen::Isometry3d tcp = Eigen::Isometry3d::Identity();
        if (!GetTcpInReference(robot_state, tcp))
        {
            tcp.translation() =
                Eigen::Vector3d(robot_state.cartesian_position[0],
                                robot_state.cartesian_position[1],
                                robot_state.cartesian_position[2]);
        }
        return tcp;
    }();
    const Eigen::Vector3d actual_world =
        (reference.world_from_reference * actual_reference).translation();
    const Eigen::Vector3d target_world =
        (reference.world_from_reference * hand.target_tcp_reference)
            .translation();
    const char *grip_hint = "";
    if (hand.require_release)
    {
        grip_hint = " wait_release";
    }
    else if (!hand.active && pose.grip > kGripReleaseThreshold &&
             pose.grip < kGripPressThreshold)
    {
        grip_hint = " grip_light";
    }
    std::cout << name << " grip=" << std::fixed << std::setprecision(2)
              << pose.grip << " clutch=" << (hand.active ? "ON" : "OFF")
              << " stick_y=" << pose.thumbstick[1]
              << " j6=" << (hand.joint6_active ? "JOINT" : "OFF") << grip_hint
              << " stream=";
    if (hand.joint6_active)
    {
        std::cout << "J6";
    }
    else if (hand.active && hand.servop_stream_started)
    {
        std::cout << "TRACK";
    }
    else if (hand.servop_brake_frames_remaining > 0 ||
             hand.joint6_brake_frames_remaining > 0)
    {
        std::cout << "BRAKE";
    }
    else
    {
        std::cout << "OFF";
    }
    std::cout << " frames=" << hand.command_frames
              << " dxyz=" << hand.translation_step * 1000.0 << "mm"
              << " drot=" << hand.rotation_step * 180.0 / kPi << "deg"
              << " cond=";
    if (singularity.fresh && std::isfinite(singularity.condition_number))
    {
        std::cout << singularity.condition_number;
    }
    else if (singularity.fresh)
    {
        std::cout << "inf";
    }
    else
    {
        std::cout << "stale";
    }
    std::cout << " sing="
              << SingularityTravelToString(hand.singularity_travel)
              << " sing_scale=" << hand.singularity_scale
              << " map_yaw=" << hand.clutch_head_yaw * 180.0 / kPi << "deg"
              << " pos_err=" << PositionErrorMillimeters(robot_state, hand)
              << "mm ori_err=" << OrientationErrorDegrees(robot_state, hand)
              << "deg";
    if (hand.active && hand.servop_stream_started &&
        PositionErrorMillimeters(robot_state, hand) > 15.0)
    {
        std::cout << " LAG";
    }
    std::cout << "\n"
              << "  ref=\"" << reference.name << "\" arm xyz=["
              << robot_state.cartesian_position[0] << ","
              << robot_state.cartesian_position[1] << ","
              << robot_state.cartesian_position[2] << "] target_arm=["
              << hand.target_tcp_reference.translation()[0] << ","
              << hand.target_tcp_reference.translation()[1] << ","
              << hand.target_tcp_reference.translation()[2] << "]\n"
              << "  world xyz=[" << actual_world[0] << "," << actual_world[1]
              << "," << actual_world[2] << "] target_world=[" << target_world[0]
              << "," << target_world[1] << "," << target_world[2] << "] dW=["
              << hand.last_world_delta[0] << "," << hand.last_world_delta[1]
              << "," << hand.last_world_delta[2] << "]" << std::endl;
}

void UpdateHandTarget(const PicoControllerPose &pose,
                      const PicoHeadsetPose &headset,
                      const rcore::RobotState &robot_state,
                      const ArmReference &reference, bool enable_rotation,
                      bool head_yaw_clutched, double translation_scale,
                      double rotation_scale, double j6_thumbstick_speed,
                      bool operator_facing_robot,
                      const SingularitySlowdownStatus &singularity,
                      HandServoState &state)
{
    if (!pose.present || !pose.tracked)
    {
        FreezeHand(robot_state, state, true);
        return;
    }
    if (pose.grip <= kGripReleaseThreshold)
    {
        if (state.active || state.joint6_active || state.require_release)
        {
            FreezeHand(robot_state, state, false);
        }
        state.require_release = false;
        return;
    }
    if (state.require_release ||
        (!state.active && pose.grip < kGripPressThreshold))
    {
        return;
    }

    const double j6_axis = NormalizeJ6Thumbstick(pose.thumbstick[1]);
    const auto now = std::chrono::steady_clock::now();
    if (std::abs(j6_axis) > 0.0)
    {
        if (!state.joint6_active &&
            !SetJointTargetFromRobotState(robot_state, state))
        {
            FreezeHand(robot_state, state, true);
            return;
        }
        if (state.joint6_active && !state.joint6_target_valid)
        {
            FreezeHand(robot_state, state, true);
            return;
        }

        if (!state.joint6_active)
        {
            // Switching from ServoP to J6 is a deliberate mode transition.
            // Start from measured joint positions so the first passthrough
            // frame cannot jump the other five joints.
            state.active = false;
            state.servop_stream_started = false;
            state.servop_brake_frames_remaining = 0;
            state.joint6_active = true;
            state.joint6_brake_frames_remaining = 0;
            state.last_joint6_update = now;
            state.command_frames = 0;
        }
        state.joint6_velocity = j6_axis * j6_thumbstick_speed;
        ++state.command_frames;
        return;
    }

    if (state.joint6_active)
    {
        // Neutral stick hands control back to the existing Grip/6DoF ServoP
        // path. The next block re-clutches ServoP from the current TCP.
        state.joint6_active = false;
        state.joint6_target_valid = false;
        state.joint6_velocity = 0.0;
        state.last_joint6_update = {};
        state.active = false;
        state.servop_stream_started = false;
        state.joint6_brake_frames_remaining = 0;
    }

    if (!reference.valid)
    {
        FreezeHand(robot_state, state, true);
        return;
    }

    Eigen::Isometry3d current_tcp_reference = Eigen::Isometry3d::Identity();
    if (!GetTcpInReference(robot_state, current_tcp_reference))
    {
        FreezeHand(robot_state, state, true);
        return;
    }

    const Eigen::Vector3d hand_position = WebXrPosition(pose);
    const Eigen::Matrix3d hand_orientation = WebXrFromGrip(pose);
    if (!state.active)
    {
        Eigen::Matrix3d robot_world_from_webxr = RobotWorldFromWebXr();
        double head_yaw = 0.0;
        if (head_yaw_clutched &&
            !RobotWorldFromHeadYaw(headset, robot_world_from_webxr, head_yaw))
        {
            FreezeHand(robot_state, state, true);
            std::cerr << "\n头显水平朝向不可用，已拒绝 Grip 并要求松开后重试。"
                      << std::endl;
            return;
        }
        robot_world_from_webxr =
            ApplyOperatorFrame(robot_world_from_webxr, operator_facing_robot);
        if (operator_facing_robot)
        {
            head_yaw = std::remainder(head_yaw + kPi, 2.0 * kPi);
        }
        state.active = true;
        // Hold the current TCP immediately. Waiting for a 1e-6 m first step
        // leaves a ServoP gap after clutch-in; the controller then coasts
        // while the headset is already moving.
        state.servop_stream_started = true;
        state.servop_brake_frames_remaining = 0;
        state.joint6_brake_frames_remaining = 0;
        state.robot_world_from_webxr_clutch = robot_world_from_webxr;
        state.clutch_head_yaw = head_yaw;
        state.hand_start_position = hand_position;
        state.filtered_hand_position = hand_position;
        state.filtered_hand_orientation =
            Eigen::Quaterniond(hand_orientation).normalized();
        state.tcp_start_world =
            reference.world_from_reference * current_tcp_reference;
        (void)SetTargetFromRobotState(robot_state, state);
        state.command_frames = 0;
        state.last_xr_update = now;
        state.singularity_scale = 1.0;
        state.singularity_scale_updated_ns = 0;
        state.singularity_travel = SingularityTravel::Nominal;
        return;
    }

    const double dt =
        std::chrono::duration<double>(now - state.last_xr_update).count();
    if (!std::isfinite(dt) || dt <= 0.0 || dt > kInputStaleSeconds)
    {
        // Keep Grip held: next fresh packet re-clutches from current TCP.
        FreezeHand(robot_state, state, false);
        return;
    }
    state.last_xr_update = now;

    Eigen::Quaterniond hand_quaternion(hand_orientation);
    hand_quaternion.normalize();
    if (state.filtered_hand_orientation.dot(hand_quaternion) < 0.0)
    {
        hand_quaternion.coeffs() *= -1.0;
    }
    const Eigen::Quaterniond previous_hand_orientation =
        state.filtered_hand_orientation;
    const double position_alpha =
        FilterAlpha(dt, kHandPositionFilterTimeConstant);
    const Eigen::Vector3d previous_hand_position = state.filtered_hand_position;
    state.filtered_hand_position +=
        position_alpha * (hand_position - state.filtered_hand_position);
    const double orientation_alpha =
        FilterAlpha(dt, kHandOrientationFilterTimeConstant);
    state.filtered_hand_orientation =
        state.filtered_hand_orientation
            .slerp(orientation_alpha, hand_quaternion)
            .normalized();

    const Eigen::Vector3d unscaled_translation_increment =
        translation_scale * (state.robot_world_from_webxr_clutch *
                             (state.filtered_hand_position -
                              previous_hand_position));

    const Eigen::Matrix3d raw_rotation_delta_webxr =
        state.filtered_hand_orientation.toRotationMatrix() *
        previous_hand_orientation.toRotationMatrix().transpose();
    const Eigen::AngleAxisd rotation_delta_axis(raw_rotation_delta_webxr);
    if (!std::isfinite(rotation_delta_axis.angle()) ||
        !rotation_delta_axis.axis().allFinite())
    {
        FreezeHand(robot_state, state, true);
        return;
    }
    const double unscaled_rotation_angle =
        enable_rotation
            ? std::min(rotation_scale * rotation_delta_axis.angle(),
                       kMaxOrientationTargetRateRadiansPerSecond * dt)
            : 0.0;
    const Eigen::Matrix3d &world_from_webxr =
        state.robot_world_from_webxr_clutch;
    const Eigen::Vector3d unscaled_rotation_increment_world =
        unscaled_rotation_angle *
        (world_from_webxr * rotation_delta_axis.axis());

    const Eigen::Matrix3d base_from_world =
        reference.world_from_reference.linear().transpose();
    Eigen::Matrix<double, 6, 1> commanded_twist_base;
    commanded_twist_base.head<3>() =
        base_from_world * unscaled_translation_increment;
    commanded_twist_base.tail<3>() =
        base_from_world * unscaled_rotation_increment_world;
    const bool approaching =
        !singularity.toward_valid ||
        IsApproachingSingularity(singularity.toward_singularity,
                                 commanded_twist_base);
    double singularity_speed_scale = ConditionNumberToSpeedScale(
        singularity.condition_number, approaching);
    if (state.singularity_scale_updated_ns > 0)
    {
        singularity_speed_scale = FilterSingularityRecoveryScale(
            singularity_speed_scale, state.singularity_scale, dt,
            kSingularityRecoveryFilterTimeConstantSeconds);
    }
    singularity_speed_scale = std::clamp(singularity_speed_scale, 0.0, 1.0);
    state.singularity_scale = singularity_speed_scale;
    state.singularity_scale_updated_ns = SteadyNowNanoseconds();
    state.singularity_travel = ClassifySingularityTravel(
        singularity.condition_number, singularity.toward_valid, approaching);

    const Eigen::Vector3d translation_increment =
        singularity_speed_scale * unscaled_translation_increment;
    const Eigen::Vector3d previous_target_world =
        (reference.world_from_reference * state.target_tcp_reference)
            .translation();
    Eigen::Vector3d translation_delta = previous_target_world +
                                        translation_increment -
                                        state.tcp_start_world.translation();
    const double translation_norm = translation_delta.norm();
    if (translation_norm > kMaxTranslationDeltaMeters)
    {
        translation_delta *= kMaxTranslationDeltaMeters / translation_norm;
    }
    state.last_world_delta = translation_delta;

    const double rotation_angle =
        singularity_speed_scale * unscaled_rotation_angle;
    const Eigen::Matrix3d rotation_delta_webxr_limited =
        Eigen::AngleAxisd(rotation_angle, rotation_delta_axis.axis())
            .toRotationMatrix();
    const Eigen::Matrix3d rotation_delta_world = world_from_webxr *
                                                 rotation_delta_webxr_limited *
                                                 world_from_webxr.transpose();

    Eigen::Isometry3d target_tcp_world = state.tcp_start_world;
    target_tcp_world.translation() =
        state.tcp_start_world.translation() + translation_delta;
    target_tcp_world.linear() =
        rotation_delta_world * (reference.world_from_reference.linear() *
                                state.target_tcp_reference.linear());

    const Eigen::Isometry3d target_tcp_reference =
        reference.world_from_reference.inverse() * target_tcp_world;
    if (!target_tcp_reference.matrix().allFinite())
    {
        FreezeHand(robot_state, state, true);
        return;
    }

    const Eigen::Isometry3d previous_target = state.target_tcp_reference;
    state.target_tcp_reference = target_tcp_reference;
    state.target_rpy = RotationToContinuousEulerZyx(
        state.target_tcp_reference.linear(), state.target_rpy);
    state.translation_step = (state.target_tcp_reference.translation() -
                              previous_target.translation())
                                 .norm();
    const Eigen::AngleAxisd rotation_step(state.target_tcp_reference.linear() *
                                          previous_target.linear().transpose());
    state.rotation_step =
        std::isfinite(rotation_step.angle()) ? rotation_step.angle() : 0.0;
    state.servop_stream_started =
        state.servop_stream_started ||
        state.translation_step >= kServoPStartTranslationMeters ||
        state.rotation_step >= kServoPStartRotationRadians;
    state.target_valid = true;
    ++state.command_frames;
}

bool WaitForControllers(Pico &pico, PicoSample &packet)
{
    std::cout << "\n等待 PICO 左右手柄进入 WebXR..." << std::endl;
    while (g_running)
    {
        if (TakePicoSample(pico, packet) &&
            SecondsSince(packet.timestamp_sec) <= kInputStaleSeconds &&
            packet.left.tracked && packet.right.tracked)
        {
            std::cout << "左右手柄 6DoF 已就绪，参考空间: "
                      << packet.reference_space << std::endl;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
}

bool WaitForReleasedGrips(Pico &pico)
{
    std::cout << "请松开左右 Grip，保持 0.5 秒..." << std::endl;
    auto released_since = std::chrono::steady_clock::time_point{};
    auto last_print = std::chrono::steady_clock::now();
    PicoSample packet;
    bool has_packet = false;
    while (g_running)
    {
        has_packet = TakePicoSample(pico, packet) || has_packet;
        const bool fresh = has_packet && SecondsSince(packet.timestamp_sec) <=
                                             kInputStaleSeconds;
        const bool released = fresh && packet.left.tracked &&
                              packet.right.tracked &&
                              packet.left.grip <= kGripReleaseThreshold &&
                              packet.right.grip <= kGripReleaseThreshold;
        if (!released)
        {
            released_since = {};
        }
        else if (released_since == std::chrono::steady_clock::time_point{})
        {
            released_since = std::chrono::steady_clock::now();
        }
        else if (SecondsSince(released_since) >= 0.5)
        {
            std::cout << "Grip 已松开，开始连接机器人。" << std::endl;
            return true;
        }
        if (SecondsSince(last_print) >= 0.5)
        {
            if (fresh)
            {
                std::cout << "等待松开 Grip: L=" << std::fixed
                          << std::setprecision(2) << packet.left.grip
                          << " R=" << packet.right.grip << std::endl;
            }
            else
            {
                std::cout << "等待新的 PICO 输入..." << std::endl;
            }
            last_print = std::chrono::steady_clock::now();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

bool ValidateHeadYawMapping()
{
    PicoHeadsetPose headset;
    headset.present = true;
    headset.tracked = true;
    Eigen::Matrix3d mapping;
    double yaw = 0.0;
    if (!RobotWorldFromHeadYaw(headset, mapping, yaw) ||
        (mapping * -Eigen::Vector3d::UnitZ() - Eigen::Vector3d::UnitX())
                .norm() > 1e-9 ||
        std::abs(yaw) > 1e-9)
    {
        return false;
    }

    const Eigen::Quaterniond turned_right(
        Eigen::AngleAxisd(-kPi / 2.0, Eigen::Vector3d::UnitY()));
    headset.orientation_xyzw = {
        turned_right.x(),
        turned_right.y(),
        turned_right.z(),
        turned_right.w(),
    };
    return RobotWorldFromHeadYaw(headset, mapping, yaw) &&
           (mapping * Eigen::Vector3d::UnitX() - Eigen::Vector3d::UnitX())
                   .norm() <= 1e-9 &&
           std::abs(yaw - kPi / 2.0) <= 1e-9 &&
           (ApplyOperatorFrame(RobotWorldFromWebXr(), true) *
                -Eigen::Vector3d::UnitZ() +
            Eigen::Vector3d::UnitX())
                   .norm() <= 1e-9 &&
           (ApplyOperatorFrame(RobotWorldFromWebXr(), true) *
                Eigen::Vector3d::UnitX() -
            Eigen::Vector3d::UnitY())
                   .norm() <= 1e-9;
}

} // namespace

int main(int argc, char **argv)
{
    Options options;
    if (!ParseOptions(argc, argv, options))
    {
        PrintUsage(argv[0]);
        return 2;
    }
    if (!options.enable_motion)
    {
        std::cerr << "PICO bridge 需要显式传入 --enable-motion。" << std::endl;
        PrintUsage(argv[0]);
        return 2;
    }
    if (!ValidateHeadYawMapping())
    {
        std::cerr << "PICO 头显 yaw 坐标映射自检失败" << std::endl;
        return 1;
    }
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    Pico pico;
    PicoOptions pico_options;
    pico_options.port = options.port;
    if (!pico.Initialize(pico_options) || !pico.Start())
    {
        std::cerr << "PICO WebXR 服务启动失败: " << pico.GetLastError()
                  << std::endl;
        return 1;
    }

    std::cout << "\n========================================\n"
              << "  Peripheral PICO SDK Bridge\n"
              << "========================================\n"
              << "PC 服务: http://127.0.0.1:" << options.port << "/\n"
              << "USB 映射: adb reverse tcp:" << options.port
              << " tcp:" << options.port << "\n"
              << "模式: SDK ServoP"
              << "\n映射: 平移 " << options.translation_scale << "x，旋转 "
              << (options.enable_rotation
                      ? std::to_string(options.rotation_scale) + "x"
                      : "关闭")
              << "\n方向基准: "
              << (options.head_yaw_clutched ? "Grip 锁定头显 yaw"
                                            : "WebXR room-fixed")
              << "\nJ6 推杆: 上下控制，速度 " << options.j6_thumbstick_speed
              << " rad/s，死区 " << kJ6ThumbstickDeadzone
              << "\n操作者坐标系: "
              << (options.operator_facing_robot ? "face-to-face" : "same-side")
              << std::endl;
    if (options.operator_facing_robot)
    {
        std::cout << "face-to-face 手柄对调: 物理右手柄→左臂，物理左手柄→右臂。"
                  << std::endl;
    }
    std::cout << "必须按住 Grip≥" << std::fixed << std::setprecision(2)
              << kGripPressThreshold << " 才会发 ServoP；松开到 ≤"
              << kGripReleaseThreshold
              << " 冻结。终端 clutch=OFF 时手柄移动不会带动机械臂。"
              << std::endl;

    PicoSample first_packet;
    if (!WaitForControllers(pico, first_packet))
    {
        pico.Stop();
        return 0;
    }
    if (!WaitForReleasedGrips(pico))
    {
        pico.Stop();
        return 0;
    }

    const UrdfMounts mounts = LoadB3UrdfMounts();
    if (mounts.valid)
    {
        std::cout << "B3 安装变换: " << mounts.path << std::endl;
    }

    rcore::sdk::DualArmRobot arms;
    if (!arms.Initialize(options.robot_ip))
    {
        std::cerr << "DualArmRobot 初始化失败" << std::endl;
        pico.Stop();
        return 1;
    }

    std::string command_source_error;
    if (!EnsureSdkCommandSource(arms.left_robot(), "L", command_source_error) ||
        !EnsureSdkCommandSource(arms.right_robot(), "R", command_source_error))
    {
        std::cerr << command_source_error << std::endl;
        arms.Shutdown();
        pico.Stop();
        return 1;
    }

    ArmReference left_reference;
    ArmReference right_reference;
    std::string reference_error;
    if (!ResolveDualReferences(arms, mounts, left_reference, right_reference,
                               reference_error))
    {
        std::cerr << "世界系解析失败: " << reference_error << std::endl;
        arms.Shutdown();
        pico.Stop();
        return 1;
    }
    const auto describe_world_ref = [](const ArmReference &ref)
    {
        const Eigen::Vector3d t = ref.world_from_reference.translation();
        const double rot_from_i =
            (ref.world_from_reference.linear() - Eigen::Matrix3d::Identity())
                .norm();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << "ref=\"" << ref.name
            << "\" W.t=[" << t[0] << "," << t[1] << "," << t[2]
            << "] |R-I|=" << rot_from_i;
        return oss.str();
    };
    std::cout << "世界系 L: " << describe_world_ref(left_reference)
              << std::endl;
    std::cout << "世界系 R: " << describe_world_ref(right_reference)
              << std::endl;
    if (IsNearIdentity(left_reference.world_from_reference) ||
        IsNearIdentity(right_reference.world_from_reference))
    {
        std::cerr << "警告: 至少一侧 world_from_reference≈I，B3 斜装未生效；"
                     "face-to-face 的 World 平移会落在臂系而非整机世界系。"
                  << std::endl;
    }

    auto enable_result = arms.Enable();
    if (!enable_result.Ok())
    {
        std::cerr << "双臂使能失败: L(" << enable_result.left.GetErrorCode()
                  << ") " << enable_result.left.GetErrorMsg() << " R("
                  << enable_result.right.GetErrorCode() << ") "
                  << enable_result.right.GetErrorMsg() << std::endl;
        arms.Shutdown();
        pico.Stop();
        return 1;
    }

    HandServoState left_hand;
    HandServoState right_hand;
    const auto initial_left_state = arms.left_robot().GetState();
    const auto initial_right_state = arms.right_robot().GetState();
    if (!SetTargetFromRobotState(initial_left_state, left_hand) ||
        !SetTargetFromRobotState(initial_right_state, right_hand))
    {
        std::cerr << "当前 TCP 状态无效，拒绝启动 PICO 遥操作。" << std::endl;
        (void)arms.Disable();
        arms.Shutdown();
        pico.Stop();
        return 1;
    }

    SingularitySlowdownMonitor left_singularity_monitor(arms.left_robot());
    SingularitySlowdownMonitor right_singularity_monitor(arms.right_robot());
    left_singularity_monitor.Start();
    right_singularity_monitor.Start();
    if (!left_singularity_monitor.WaitForInitialSample(
            std::chrono::seconds(1)) ||
        !right_singularity_monitor.WaitForInitialSample(
            std::chrono::seconds(1)))
    {
        std::cerr << "1s 内未取得双臂 Jacobian，拒绝启动 PICO ServoP。"
                  << std::endl;
        left_singularity_monitor.Stop();
        right_singularity_monitor.Stop();
        (void)arms.Disable();
        arms.Shutdown();
        pico.Stop();
        return 1;
    }

    std::cout << "PICO 遥操作已启用：Grip 按住控制，松开冻结；"
                 "Trigger 二值映射夹爪（0=开，1=关）；"
                 "推杆上下进入本臂 J6 关节透传；"
                 "ServoP 按 Jacobian 条件数减速：朝奇异走时超过 "
              << kSingularitySoftConditionNumber
              << " 按 sqrt(" << kSingularitySoftConditionNumber
              << "/condition) 缩放，离开时门槛×"
              << kLeavingSingularityThresholdMultiplier
              << "。" << std::endl;

    RobotStateFreshnessWatchdog left_state_watchdog;
    RobotStateFreshnessWatchdog right_state_watchdog;
    const auto initial_observed_at = std::chrono::steady_clock::now();
    (void)left_state_watchdog.Observe(initial_left_state, initial_observed_at);
    (void)right_state_watchdog.Observe(initial_right_state,
                                       initial_observed_at);

    auto next_servo_tick = std::chrono::steady_clock::now();
    auto last_print = next_servo_tick;
    uint64_t last_sequence = first_packet.sequence;
    bool input_was_fresh = true;
    bool robot_state_was_fresh = true;
    bool left_singularity_was_fresh = true;
    bool right_singularity_was_fresh = true;
    bool safety_fault_detected = false;
    PicoSample packet = first_packet;
    rcore::GripperCommand last_left_gripper = rcore::GripperCommand::Open;
    rcore::GripperCommand last_right_gripper = rcore::GripperCommand::Open;
    bool left_gripper_initialized = false;
    bool right_gripper_initialized = false;

    while (g_running.load(std::memory_order_acquire))
    {
        (void)TakePicoSample(pico, packet);
        const bool fresh =
            pico.IsConnected() &&
            SecondsSince(packet.timestamp_sec) <= kInputStaleSeconds;
        const auto left_state = arms.left_robot().GetState();
        const auto right_state = arms.right_robot().GetState();
        const SingularitySlowdownStatus left_singularity =
            left_singularity_monitor.Snapshot();
        const SingularitySlowdownStatus right_singularity =
            right_singularity_monitor.Snapshot();

        if (left_state.errors.GetLatchedErrorCount() > 0 ||
            right_state.errors.GetLatchedErrorCount() > 0)
        {
            safety_fault_detected = true;
            std::cerr << "\n检测到控制器锁存安全错误，停止双臂命令：";
            PrintLatchedErrors("L", left_state);
            PrintLatchedErrors("R", right_state);
            std::cerr << std::endl;
            break;
        }

        const auto observed_at = std::chrono::steady_clock::now();
        const bool states_fresh =
            left_state_watchdog.Observe(left_state, observed_at) &&
            right_state_watchdog.Observe(right_state, observed_at);
        if (!states_fresh)
        {
            if (robot_state_was_fresh)
            {
                FreezeHand(left_state, left_hand, false);
                FreezeHand(right_state, right_hand, false);
                std::cerr << "\nRobotState 超过 "
                          << static_cast<int>(kRobotStateStaleSeconds * 1000.0)
                          << "ms 未刷新，停止双臂命令；状态恢复后按住 Grip "
                             "会从当前 TCP 重新离合。"
                          << std::endl;
            }
            robot_state_was_fresh = false;
            next_servo_tick += kServoPeriod;
            std::this_thread::sleep_until(next_servo_tick);
            continue;
        }
        if (!robot_state_was_fresh)
        {
            FreezeHand(left_state, left_hand, false);
            FreezeHand(right_state, right_hand, false);
            std::cout
                << "\nRobotState 已恢复，按住 Grip 将从当前 TCP 重新离合。"
                << std::endl;
        }
        robot_state_was_fresh = true;

        const auto update_singularity_freshness =
            [&](const char *arm, const SingularitySlowdownStatus &status,
                bool &was_fresh, const rcore::RobotState &robot_state,
                HandServoState &hand)
        {
            if (!status.fresh && was_fresh)
            {
                FreezeHand(robot_state, hand, false);
                std::cerr << "\n"
                          << arm << " Jacobian 条件数超过 "
                          << static_cast<int>(kSingularityMonitorStaleSeconds *
                                              1000.0)
                          << "ms 未刷新，已冻结该臂 ServoP。" << std::endl;
            }
            else if (status.fresh && !was_fresh)
            {
                FreezeHand(robot_state, hand, false);
                std::cout << "\n"
                          << arm
                          << " Jacobian 条件数监测已恢复，Grip 将从当前 TCP "
                             "重新离合。"
                          << std::endl;
            }
            was_fresh = status.fresh;
        };
        update_singularity_freshness("L", left_singularity,
                                     left_singularity_was_fresh, left_state,
                                     left_hand);
        update_singularity_freshness("R", right_singularity,
                                     right_singularity_was_fresh, right_state,
                                     right_hand);

        if (!fresh && input_was_fresh)
        {
            FreezeHand(left_state, left_hand, false);
            FreezeHand(right_state, right_hand, false);
            std::cerr << "\nPICO 输入超过 "
                      << static_cast<int>(kInputStaleSeconds * 1000.0)
                      << "ms 未刷新，双臂已冻结；恢复后按住 Grip 会重新离合。"
                      << std::endl;
        }
        if (fresh && packet.sequence != last_sequence)
        {
            const PicoControllerPose &left_controller =
                options.operator_facing_robot ? packet.right : packet.left;
            const PicoControllerPose &right_controller =
                options.operator_facing_robot ? packet.left : packet.right;
            if (left_singularity.fresh)
            {
                UpdateHandTarget(
                    left_controller, packet.headset, left_state, left_reference,
                    options.enable_rotation, options.head_yaw_clutched,
                    options.translation_scale, options.rotation_scale,
                    options.j6_thumbstick_speed, options.operator_facing_robot,
                    left_singularity, left_hand);
            }
            if (right_singularity.fresh)
            {
                UpdateHandTarget(
                    right_controller, packet.headset, right_state,
                    right_reference, options.enable_rotation,
                    options.head_yaw_clutched, options.translation_scale,
                    options.rotation_scale, options.j6_thumbstick_speed,
                    options.operator_facing_robot, right_singularity,
                    right_hand);
            }
            const auto left_gripper =
                TriggerToGripperCommand(left_controller.trigger);
            const auto right_gripper =
                TriggerToGripperCommand(right_controller.trigger);
            if (!left_gripper_initialized || left_gripper != last_left_gripper)
            {
                auto left_gripper_result = PublishPicoGripperCommand(
                    arms.left_robot(), left_controller.trigger,
                    packet.timestamp_sec);
                if (!left_gripper_result.IsSuccess())
                {
                    std::cerr << "PICO Trigger 夹爪命令发布失败: L("
                              << left_gripper_result.GetErrorCode() << ") "
                              << left_gripper_result.GetErrorMsg() << std::endl;
                }
                else
                {
                    last_left_gripper = left_gripper;
                    left_gripper_initialized = true;
                }
            }
            if (!right_gripper_initialized ||
                right_gripper != last_right_gripper)
            {
                auto right_gripper_result = PublishPicoGripperCommand(
                    arms.right_robot(), right_controller.trigger,
                    packet.timestamp_sec);
                if (!right_gripper_result.IsSuccess())
                {
                    std::cerr << "PICO Trigger 夹爪命令发布失败: R("
                              << right_gripper_result.GetErrorCode() << ") "
                              << right_gripper_result.GetErrorMsg()
                              << std::endl;
                }
                else
                {
                    last_right_gripper = right_gripper;
                    right_gripper_initialized = true;
                }
            }
            last_sequence = packet.sequence;
        }
        input_was_fresh = fresh;

        // Do not call GetBasePose/GetControllerConfig RPC on the 1kHz ServoP
        // loop. A hung or slow RPC stalls the stream and freezes both arms.
        // Re-resolve only when RobotState.reference_frame actually changes.
        if (left_state.reference_frame != left_reference.name ||
            right_state.reference_frame != right_reference.name)
        {
            ArmReference new_left;
            ArmReference new_right;
            std::string error;
            if (ResolveDualReferences(arms, mounts, new_left, new_right, error))
            {
                left_reference = new_left;
                right_reference = new_right;
                FreezeHand(left_state, left_hand, false);
                FreezeHand(right_state, right_hand, false);
                std::cout << "\n参考系已变化: L=" << left_reference.name
                          << " R=" << right_reference.name
                          << "，按住 Grip 将从当前 TCP 重新离合。" << std::endl;
            }
            else
            {
                left_reference.valid = false;
                right_reference.valid = false;
                FreezeHand(left_state, left_hand, true);
                FreezeHand(right_state, right_hand, true);
                std::cerr << "\n世界系刷新失败: " << error << std::endl;
            }
        }

        (void)SendSdkServoPTick(arms.left_robot(), left_hand);
        (void)SendSdkServoPTick(arms.right_robot(), right_hand);

        if (SecondsSince(last_print) >= 1.0)
        {
            const PicoControllerPose &left_controller =
                options.operator_facing_robot ? packet.right : packet.left;
            const PicoControllerPose &right_controller =
                options.operator_facing_robot ? packet.left : packet.right;
            std::cout << "XR age="
                      << (fresh ? SecondsSince(packet.timestamp_sec) * 1000.0
                                : -1.0)
                      << "ms ref=" << packet.reference_space << std::endl;
            PrintArmDiagnostics("L", left_controller, left_state, left_hand,
                                left_reference, left_singularity);
            PrintArmDiagnostics("R", right_controller, right_state, right_hand,
                                right_reference, right_singularity);
            last_print = std::chrono::steady_clock::now();
        }

        next_servo_tick += kServoPeriod;
        const auto now = std::chrono::steady_clock::now();
        if (next_servo_tick < now - kServoPeriod)
        {
            next_servo_tick = now;
        }
        std::this_thread::sleep_until(next_servo_tick);
    }

    left_singularity_monitor.Stop();
    right_singularity_monitor.Stop();
    std::cout << "\n正在锁定当前关节并分别下使能..." << std::endl;
    try
    {
        if (!safety_fault_detected)
        {
            HoldThenDisable(arms.left_robot(), "L");
            HoldThenDisable(arms.right_robot(), "R");
        }
        else
        {
            auto left_disable = arms.left_robot().Disable();
            auto right_disable = arms.right_robot().Disable();
            PrintDisableStatus("L", left_disable,
                               arms.left_robot().Control().GetMotorStatus());
            PrintDisableStatus("R", right_disable,
                               arms.right_robot().Control().GetMotorStatus());
        }
    }
    catch (const std::exception &exception)
    {
        std::cerr << "双臂后端已离线，Disable 未完成: " << exception.what()
                  << std::endl;
    }
    try
    {
        arms.Shutdown();
    }
    catch (const std::exception &exception)
    {
        std::cerr << "双臂 Shutdown 清理异常: " << exception.what()
                  << std::endl;
    }
    pico.Stop();
    std::cout << "PICO 双臂遥操作已结束。" << std::endl;
    return 0;
}
