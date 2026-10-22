/**
 * @file singularity_scale.hpp
 * @brief PICO ServoP 近奇异速度缩放：条件数 + 朝奇异/离开（MoveIt Servo 同构）
 *
 * MoveIt `velocityScalingFactorForSingularity`（moveit2
 * `moveit_ros/moveit_servo/src/utils/common.cpp`）：
 * - 几何雅可比 SVD，κ = σ_max / σ_min；
 * - U 最后一列是最弱笛卡尔方向，SVD 符号不定，需用 look-ahead / 观测运动定号；
 * - 指令 twist 与该方向点积 > 0 视为朝奇异走，否则视为离开；
 * - 离开时门槛乘 `leaving_singularity_threshold_multiplier`（默认 2）。
 *
 * 本模块是无状态纯函数。PICO 桥靠近奇异点时在条件数 30..60 二次减速到零；
 * 离开时门槛乘 2，在相同姿态获得恢复速度，避免停在边界后无法退出。
 * 到达边界只冻结继续靠近的运动，不终止遥操会话。
 * SDK 没有 J(q)，候选目标检查通过 FK 数值雅可比完成。

 */

#pragma once

#include <Eigen/Core>
#include <Eigen/SVD>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace smrcore
{
namespace peripherals
{

constexpr double kSingularitySoftConditionNumber = 30.0;
constexpr double kSingularityHardConditionNumber = 60.0;
constexpr double kLeavingSingularityThresholdMultiplier = 2.0;
constexpr double kSingularityRecoveryFilterTimeConstantSeconds = 0.20;
constexpr double kSingularityTwistAlignEpsilon = 1e-4;
constexpr double kSingularityConditionDeltaEpsilon = 1.0;

enum class SingularityTravel : std::uint8_t
{
    Nominal = 0,
    Approaching = 1,
    Leaving = 2,
    Halt = 3,
};

struct JacobianSvd
{
    bool valid = false;
    double condition_number = std::numeric_limits<double>::infinity();
    Eigen::Matrix<double, 6, 1> least_singular_direction =
        Eigen::Matrix<double, 6, 1>::Zero();
};

inline const char *SingularityTravelToString(SingularityTravel travel)
{
    switch (travel)
    {
    case SingularityTravel::Nominal:
        return "OK";
    case SingularityTravel::Approaching:
        return "APPR";
    case SingularityTravel::Leaving:
        return "LEAVE";
    case SingularityTravel::Halt:
        return "HALT";
    }
    return "?";
}

inline double JacobianConditionNumberFromSingularValues(double sigma_max,
                                                        double sigma_min)
{
    if (!(sigma_max > 0.0) ||
        !(sigma_min > std::numeric_limits<double>::epsilon() * sigma_max))
    {
        return std::numeric_limits<double>::infinity();
    }
    return sigma_max / sigma_min;
}

inline JacobianSvd AnalyzeJacobian(const Eigen::Matrix<double, 6, 6> &jacobian)
{
    JacobianSvd result;
    if (!jacobian.allFinite())
    {
        return result;
    }

    const Eigen::JacobiSVD<Eigen::Matrix<double, 6, 6>> svd(
        jacobian, Eigen::ComputeFullU | Eigen::ComputeFullV);
    const auto &singular_values = svd.singularValues();
    result.condition_number = JacobianConditionNumberFromSingularValues(
        singular_values[0], singular_values[5]);
    result.least_singular_direction = svd.matrixU().col(5);
    result.valid = true;
    return result;
}

/// MoveIt look-ahead：沿 U 末列走一步后 κ 下降，则该列指向离开，需翻转。
inline Eigen::Matrix<double, 6, 1> ResolveTowardSingularityDirection(
    Eigen::Matrix<double, 6, 1> singular_direction, double current_condition,
    double look_ahead_condition)
{
    if (std::isfinite(current_condition) &&
        std::isfinite(look_ahead_condition) &&
        look_ahead_condition <= current_condition)
    {
        singular_direction *= -1.0;
    }
    return singular_direction;
}

/// SDK 无 J(q+dq) 时：用已发生的 Base 系 twist 与 Δκ 给 U 末列定号。
inline Eigen::Matrix<double, 6, 1>
ResolveTowardFromObservedMotion(Eigen::Matrix<double, 6, 1> singular_direction,
                                const Eigen::Matrix<double, 6, 1> &base_twist,
                                double current_condition,
                                double previous_condition)
{
    if (!std::isfinite(current_condition) || !std::isfinite(previous_condition))
    {
        return singular_direction;
    }
    if (std::abs(current_condition - previous_condition) <=
        kSingularityConditionDeltaEpsilon)
    {
        return singular_direction;
    }
    const double along = singular_direction.dot(base_twist);
    if (!(std::abs(along) > kSingularityTwistAlignEpsilon))
    {
        return singular_direction;
    }
    const bool kappa_increased = current_condition > previous_condition;
    const bool moved_along_direction = along > 0.0;
    if (moved_along_direction != kappa_increased)
    {
        singular_direction *= -1.0;
    }
    return singular_direction;
}

inline Eigen::Matrix<double, 6, 1>
AlignSingularDirection(Eigen::Matrix<double, 6, 1> direction,
                       const Eigen::Matrix<double, 6, 1> &previous)
{
    if (previous.dot(direction) < 0.0)
    {
        direction *= -1.0;
    }
    return direction;
}

inline bool
IsApproachingSingularity(const Eigen::Matrix<double, 6, 1> &toward_singularity,
                         const Eigen::Matrix<double, 6, 1> &commanded_twist)
{
    return toward_singularity.dot(commanded_twist) > 0.0;
}

inline double ConditionNumberToSpeedScale(double condition_number,
                                          bool approaching)
{
    if (!std::isfinite(condition_number))
    {
        return 0.0;
    }
    const double threshold_multiplier =
        approaching ? 1.0 : kLeavingSingularityThresholdMultiplier;
    const double soft = kSingularitySoftConditionNumber * threshold_multiplier;
    const double hard = kSingularityHardConditionNumber * threshold_multiplier;
    const double linear =
        std::clamp((hard - condition_number) / (hard - soft), 0.0, 1.0);
    return approaching ? linear * linear : linear;
}

inline SingularityTravel ClassifySingularityTravel(double condition_number,
                                                   bool toward_valid,
                                                   bool approaching)
{
    if (!std::isfinite(condition_number) ||
        condition_number >= kSingularityHardConditionNumber)
    {
        return SingularityTravel::Halt;
    }
    if (condition_number <= kSingularitySoftConditionNumber)
    {
        return SingularityTravel::Nominal;
    }
    if (!toward_valid || approaching)
    {
        return SingularityTravel::Approaching;
    }
    return SingularityTravel::Leaving;
}

// B3 i3 elbow/wrist axes: reject a neighbourhood of q3/q5 = n*pi.
// IK branch continuity is handled by the backend ServoP IK, not here.
inline bool GuardJointCandidate(const std::array<double, 6> &actual,
                                const std::array<double, 6> &candidate)
{
    constexpr double clearance_sine = 0.173648177667; // sin(10 degrees)
    for (size_t i = 0; i < 6; ++i)
    {
        if (!std::isfinite(actual[i]) || !std::isfinite(candidate[i]))
            return false;
    }
    for (size_t i : {size_t{2}, size_t{4}})
    {
        const double a = std::sin(actual[i]), b = std::sin(candidate[i]);
        if (std::abs(a) < clearance_sine || std::abs(b) < clearance_sine ||
            a * b <= 0.0)
            return false;
    }
    return true;
}

/// 降速立刻生效；只有 scale 回升才按时间常数滤波，避免突然放开。
inline double FilterSingularityRecoveryScale(double scale,
                                             double previous_scale, double dt,
                                             double time_constant)
{
    if (!(scale > previous_scale) || !std::isfinite(dt) || dt <= 0.0 ||
        !std::isfinite(time_constant) || time_constant <= 0.0)
    {
        return scale;
    }
    const double alpha = 1.0 - std::exp(-dt / time_constant);
    return previous_scale +
           std::clamp(alpha, 0.0, 1.0) * (scale - previous_scale);
}

} // namespace peripherals
} // namespace smrcore
