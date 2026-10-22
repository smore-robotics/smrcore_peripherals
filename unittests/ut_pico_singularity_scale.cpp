#include "peripherals/pico/singularity_scale.hpp"

#include <cassert>
#include <cmath>
#include <limits>

using smrcore::peripherals::AlignSingularDirection;
using smrcore::peripherals::AnalyzeJacobian;
using smrcore::peripherals::ClassifySingularityTravel;
using smrcore::peripherals::ConditionNumberToSpeedScale;
using smrcore::peripherals::FilterSingularityRecoveryScale;
using smrcore::peripherals::IsApproachingSingularity;
using smrcore::peripherals::kLeavingSingularityThresholdMultiplier;
using smrcore::peripherals::kSingularityRecoveryFilterTimeConstantSeconds;
using smrcore::peripherals::kSingularitySoftConditionNumber;
using smrcore::peripherals::ResolveTowardFromObservedMotion;
using smrcore::peripherals::ResolveTowardSingularityDirection;
using smrcore::peripherals::SingularityTravel;
using smrcore::peripherals::SingularityTravelToString;

int main()
{
    const Eigen::Matrix<double, 6, 6> identity =
        Eigen::Matrix<double, 6, 6>::Identity();
    const auto identity_svd = AnalyzeJacobian(identity);
    assert(identity_svd.valid);
    assert(std::abs(identity_svd.condition_number - 1.0) < 1e-12);
    assert(std::abs(identity_svd.least_singular_direction.norm() - 1.0) <
           1e-12);

    Eigen::Matrix<double, 6, 6> stretched = identity;
    stretched(0, 0) = 4.0;
    stretched(5, 5) = 0.5;
    const auto stretched_svd = AnalyzeJacobian(stretched);
    assert(stretched_svd.valid);
    assert(std::abs(stretched_svd.condition_number - 8.0) < 1e-9);

    assert(!AnalyzeJacobian(Eigen::Matrix<double, 6, 6>::Constant(
                                std::numeric_limits<double>::quiet_NaN()))
                .valid);

    assert(ConditionNumberToSpeedScale(20.0, true) == 1.0);
    assert(ConditionNumberToSpeedScale(30.0, true) == 1.0);
    assert(ConditionNumberToSpeedScale(45.0, true) == 0.25);
    assert(ConditionNumberToSpeedScale(60.0, true) == 0.0);
    assert(ConditionNumberToSpeedScale(60.0, false) == 1.0);
    assert(ConditionNumberToSpeedScale(90.0, false) == 0.5);
    assert(ConditionNumberToSpeedScale(120.0, false) == 0.0);
    for (bool approaching : {false, true})
    {
        assert(ConditionNumberToSpeedScale(310.73, approaching) == 0.0);
        assert(
            ConditionNumberToSpeedScale(std::numeric_limits<double>::infinity(),
                                        approaching) == 0.0);
        assert(ConditionNumberToSpeedScale(
                   std::numeric_limits<double>::quiet_NaN(), approaching) ==
               0.0);
    }
    std::array<double, 6> actual{0, -1, 0.5, -1, 0.5, 0};
    auto candidate = actual;
    candidate[0] += 0.01;
    assert(smrcore::peripherals::GuardJointCandidate(actual, candidate));
    candidate = actual;
    candidate[3] += 0.06; // no fixed per-frame joint step limit
    assert(smrcore::peripherals::GuardJointCandidate(actual, candidate));
    candidate = actual;
    candidate[2] = -0.5; // elbow crosses q3 = 0
    assert(!smrcore::peripherals::GuardJointCandidate(actual, candidate));
    candidate = actual;
    actual[2] = candidate[2] = 0.065745; // captured hazardous B3 posture
    assert(!smrcore::peripherals::GuardJointCandidate(actual, candidate));
    actual[2] = candidate[2] = 0.5;
    actual[4] = candidate[4] = 3.141592653589793;
    assert(!smrcore::peripherals::GuardJointCandidate(actual, candidate));
    candidate[0] = std::numeric_limits<double>::quiet_NaN();
    assert(!smrcore::peripherals::GuardJointCandidate(actual, candidate));

    Eigen::Matrix<double, 6, 1> toward = Eigen::Matrix<double, 6, 1>::Zero();
    toward[2] = 1.0;
    const auto flipped = ResolveTowardSingularityDirection(toward, 100.0, 90.0);
    assert(flipped[2] == -1.0);
    const auto kept = ResolveTowardSingularityDirection(toward, 100.0, 110.0);
    assert(kept[2] == 1.0);

    Eigen::Matrix<double, 6, 1> twist = Eigen::Matrix<double, 6, 1>::Zero();
    twist[2] = 0.05;
    const auto observed_toward =
        ResolveTowardFromObservedMotion(toward, twist, 120.0, 90.0);
    assert(observed_toward[2] == 1.0);
    const auto observed_away =
        ResolveTowardFromObservedMotion(toward, twist, 90.0, 120.0);
    assert(observed_away[2] == -1.0);

    const auto aligned = AlignSingularDirection(-toward, toward);
    assert(aligned[2] == 1.0);

    assert(IsApproachingSingularity(toward, twist));
    assert(!IsApproachingSingularity(toward, -twist));
    assert(
        !IsApproachingSingularity(toward, Eigen::Matrix<double, 6, 1>::Zero()));

    assert(ClassifySingularityTravel(20.0, true, true) ==
           SingularityTravel::Nominal);
    assert(ClassifySingularityTravel(45.0, true, true) ==
           SingularityTravel::Approaching);
    assert(ClassifySingularityTravel(45.0, true, false) ==
           SingularityTravel::Leaving);
    assert(ClassifySingularityTravel(45.0, false, false) ==
           SingularityTravel::Approaching);
    assert(ClassifySingularityTravel(std::numeric_limits<double>::infinity(),
                                     true, false) == SingularityTravel::Halt);
    assert(SingularityTravelToString(SingularityTravel::Leaving)[0] == 'L');

    const double recovered = FilterSingularityRecoveryScale(
        1.0, 0.2, 0.20, kSingularityRecoveryFilterTimeConstantSeconds);
    const double expected = 0.2 + (1.0 - std::exp(-1.0)) * (1.0 - 0.2);
    assert(std::abs(recovered - expected) < 1e-12);
    assert(FilterSingularityRecoveryScale(
               0.2, 1.0, 0.20, kSingularityRecoveryFilterTimeConstantSeconds) ==
           0.2);

    return 0;
}
