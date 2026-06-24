#include "peripherals/spacemouse/sample_model.hpp"

#include <cassert>
#include <cmath>

using smrcore::peripherals::GripperCommand;
using smrcore::peripherals::SpaceMouseOptions;
using smrcore::peripherals::internal::SampleModel;

int main()
{
    SpaceMouseOptions options;
    options.deadzone = 0.1;
    SampleModel model(options);

    model.ApplyRawState({{350, 20, -350, 0, 0, 0}}, 0, 1.0);
    model.Tick(1.1);
    auto sample = model.TakeSample();
    assert(sample.has_value());
    assert(std::abs(sample->x - 1.0) < 1e-12);
    assert(sample->y == 0.0);
    assert(std::abs(sample->z - 1.0) < 1e-12);
    assert(!model.TakeSample().has_value());

    model.Tick(1.2);
    sample = model.TakeSample();
    assert(sample.has_value());
    assert(std::abs(sample->x - 1.0) < 1e-12);

    model.ApplyRawState({{0, 0, 0, 0, 0, 0}}, 1u << 0, 2.0);
    model.Tick(2.1);
    sample = model.TakeSample();
    assert(sample->gripper_command == GripperCommand::Close);
    model.ApplyRawState({{0, 0, 0, 0, 0, 0}}, (1u << 0) | (1u << 1), 2.2);
    model.Tick(2.3);
    sample = model.TakeSample();
    assert(sample->gripper_command == GripperCommand::Close);
    model.ApplyRawState({{0, 0, 0, 0, 0, 0}}, 0, 2.4);
    model.ApplyRawState({{0, 0, 0, 0, 0, 0}}, 1u << 1, 2.5);
    model.Tick(2.6);
    sample = model.TakeSample();
    assert(sample->gripper_command == GripperCommand::Open);
    return 0;
}
