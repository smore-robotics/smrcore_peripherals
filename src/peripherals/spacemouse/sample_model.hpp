/**
 * @file sample_model.hpp
 * @brief SpaceMouse 定频采样与归一化状态模型（内部实现）
 */

#pragma once

#include "common/latest_slot.hpp"
#include "peripherals/types.hpp"

#include <array>
#include <optional>

namespace smrcore::peripherals::internal
{

class SampleModel
{
public:
    explicit SampleModel(SpaceMouseOptions options = {});

    void Configure(const SpaceMouseOptions &options);
    void Reset();

    void ApplyRawState(const std::array<int32_t, 6> &axes,
                       uint32_t buttons,
                       double timestamp_sec);

    void Tick(double timestamp_sec);

    std::optional<SpaceMouseSample> TakeSample();
    std::optional<SpaceMouseRawSample> TakeRawSample();

private:
    SpaceMouseSample BuildSample(double timestamp_sec) const;
    SpaceMouseRawSample BuildRawSample(double timestamp_sec) const;
    double NormalizeAxis(std::size_t out_axis) const;
    void UpdateGripperLatch(uint32_t buttons);

    SpaceMouseOptions options_;
    std::array<int32_t, 6> axes_{{0, 0, 0, 0, 0, 0}};
    uint32_t buttons_{0};
    uint32_t previous_buttons_{0};
    GripperCommand gripper_command_{GripperCommand::Open};
    smrcore::peripherals::internal::LatestSlot<SpaceMouseSample> sample_slot_;
    smrcore::peripherals::internal::LatestSlot<SpaceMouseRawSample> raw_slot_;
};

} // namespace smrcore::peripherals::internal
