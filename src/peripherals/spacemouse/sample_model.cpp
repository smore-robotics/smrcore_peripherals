/**
 * @file sample_model.cpp
 * @brief SpaceMouse 定频采样与归一化状态模型实现
 */

#include "peripherals/spacemouse/sample_model.hpp"

#include <algorithm>
#include <cmath>

namespace smrcore::peripherals::internal
{

namespace
{

constexpr uint32_t kCloseButtonMask = 1u << 0;
constexpr uint32_t kOpenButtonMask = 1u << 1;

double ClampUnit(double value)
{
    return std::max(-1.0, std::min(1.0, value));
}

} // namespace

SampleModel::SampleModel(SpaceMouseOptions options)
    : options_(std::move(options))
{
}

void SampleModel::Configure(const SpaceMouseOptions &options)
{
    options_ = options;
    Reset();
}

void SampleModel::Reset()
{
    axes_ = {{0, 0, 0, 0, 0, 0}};
    buttons_ = 0;
    previous_buttons_ = 0;
    gripper_command_ = GripperCommand::Open;
    sample_slot_.Clear();
    raw_slot_.Clear();
}

void SampleModel::ApplyRawState(const std::array<int32_t, 6> &axes,
                                uint32_t buttons,
                                double timestamp_sec)
{
    axes_ = axes;
    buttons_ = buttons;
    UpdateGripperLatch(buttons_);
    raw_slot_.Set(BuildRawSample(timestamp_sec));
}

void SampleModel::Tick(double timestamp_sec)
{
    sample_slot_.Set(BuildSample(timestamp_sec));
    raw_slot_.Set(BuildRawSample(timestamp_sec));
}

std::optional<SpaceMouseSample> SampleModel::TakeSample()
{
    return sample_slot_.Take();
}

std::optional<SpaceMouseRawSample> SampleModel::TakeRawSample()
{
    return raw_slot_.Take();
}

SpaceMouseSample SampleModel::BuildSample(double timestamp_sec) const
{
    SpaceMouseSample sample;
    sample.x = NormalizeAxis(0);
    sample.y = NormalizeAxis(1);
    sample.z = NormalizeAxis(2);
    sample.roll = NormalizeAxis(3);
    sample.pitch = NormalizeAxis(4);
    sample.yaw = NormalizeAxis(5);
    sample.gripper_command = gripper_command_;
    sample.timestamp_sec = timestamp_sec;
    return sample;
}

SpaceMouseRawSample SampleModel::BuildRawSample(double timestamp_sec) const
{
    SpaceMouseRawSample sample;
    sample.axes = axes_;
    sample.buttons = buttons_;
    sample.timestamp_sec = timestamp_sec;
    return sample;
}

double SampleModel::NormalizeAxis(std::size_t out_axis) const
{
    if (out_axis >= axes_.size())
    {
        return 0.0;
    }
    const int in_axis = options_.axis_map[out_axis];
    if (in_axis < 0 || in_axis >= static_cast<int>(axes_.size()))
    {
        return 0.0;
    }
    const double scale = std::abs(options_.axis_scale[out_axis]) > 1e-9
                             ? std::abs(options_.axis_scale[out_axis])
                             : 1.0;
    double value = static_cast<double>(axes_[static_cast<std::size_t>(in_axis)]) /
                   scale;
    value *= options_.axis_sign[out_axis] >= 0 ? 1.0 : -1.0;
    value = ClampUnit(value);
    if (std::abs(value) < options_.deadzone)
    {
        value = 0.0;
    }
    return value;
}

void SampleModel::UpdateGripperLatch(uint32_t buttons)
{
    const uint32_t rising = buttons & ~previous_buttons_;
    previous_buttons_ = buttons;

    const bool close_pressed = (buttons & kCloseButtonMask) != 0;
    const bool open_pressed = (buttons & kOpenButtonMask) != 0;
    if (close_pressed && open_pressed)
    {
        return;
    }

    const bool close_edge = (rising & kCloseButtonMask) != 0;
    const bool open_edge = (rising & kOpenButtonMask) != 0;
    if (close_edge == open_edge)
    {
        return;
    }
    gripper_command_ =
        close_edge ? GripperCommand::Close : GripperCommand::Open;
}

} // namespace smrcore::peripherals::internal
