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
 * 本模块是无状态纯函数。PICO 桥阈值仍与笛卡尔规划 legacy 软限对齐（80），
 * 缩放律保持 `sqrt(threshold / κ)`（不是 MoveIt 的 17–30 线性插值，也不在
 * κ=30 硬停——离开奇异时遥操作必须还能动）。SDK 没有 J(q+dq)，桥接侧用
 * 上一拍 Base 系 TCP 速度与 Δκ 代替 MoveIt 的虚拟微扰定号。
 */

#pragma once

#include <Eigen/Core>
#include <Eigen/SVD>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace smrcore
{
namespace peripherals
{

constexpr double kSingularitySoftConditionNumber = 80.0;
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
inline Eigen::Matrix<double, 6, 1> ResolveTowardFromObservedMotion(
    Eigen::Matrix<double, 6, 1> singular_direction,
    const Eigen::Matrix<double, 6, 1> &base_twist, double current_condition,
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
    const double threshold =
        approaching ? kSingularitySoftConditionNumber
                    : kSingularitySoftConditionNumber *
                          kLeavingSingularityThresholdMultiplier;
    if (condition_number <= threshold)
    {
        return 1.0;
    }
    return std::clamp(std::sqrt(threshold / condition_number), 0.0, 1.0);
}

inline SingularityTravel ClassifySingularityTravel(double condition_number,
                                                   bool toward_valid,
                                                   bool approaching)
{
    if (!std::isfinite(condition_number))
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
