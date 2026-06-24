#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <peripherals/peripherals.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace py = pybind11;

namespace {

#ifndef SMRCORE_PERIPHERALS_VERSION_STR
#define SMRCORE_PERIPHERALS_VERSION_STR "0.0.0"
#endif

constexpr std::size_t kAxisCount = 6;

void require_six_ints(const std::vector<int>& values, const char* name) {
    if (values.size() != kAxisCount) {
        throw py::value_error(std::string(name) + " must contain exactly 6 values");
    }
}

void require_six_doubles(const std::vector<double>& values, const char* name) {
    if (values.size() != kAxisCount) {
        throw py::value_error(std::string(name) + " must contain exactly 6 values");
    }
}

void assign_array6_int(const std::vector<int>& values,
                       std::array<int, kAxisCount>* out,
                       const char* name) {
    require_six_ints(values, name);
    for (std::size_t i = 0; i < kAxisCount; ++i) {
        (*out)[i] = values[i];
    }
}

void assign_array6_double(const std::vector<double>& values,
                          std::array<double, kAxisCount>* out,
                          const char* name) {
    require_six_doubles(values, name);
    for (std::size_t i = 0; i < kAxisCount; ++i) {
        (*out)[i] = values[i];
    }
}

void apply_common_options(const py::dict& d,
                          smrcore::peripherals::PeripheralOptions* options) {
    if (d.contains("sample_rate_hz")) {
        options->sample_rate_hz = py::cast<double>(d["sample_rate_hz"]);
    }
}

smrcore::peripherals::FtSensorOptions ft_sensor_options_from_dict(const py::dict& d) {
    smrcore::peripherals::FtSensorOptions options;
    apply_common_options(d, &options);
    if (d.contains("serial_port")) {
        options.serial_port = py::cast<std::string>(d["serial_port"]);
    }
    if (d.contains("baud_rate")) {
        options.baud_rate = py::cast<int>(d["baud_rate"]);
    }
    if (d.contains("read_buffer_size")) {
        options.read_buffer_size = py::cast<std::size_t>(d["read_buffer_size"]);
    }
    if (d.contains("sensor_type")) {
        options.sensor_type = py::cast<std::string>(d["sensor_type"]);
    }
    if (d.contains("stale_timeout_ms")) {
        options.stale_timeout_ms = py::cast<int>(d["stale_timeout_ms"]);
    }
    return options;
}

smrcore::peripherals::SpaceMouseOptions spacemouse_options_from_dict(const py::dict& d) {
    smrcore::peripherals::SpaceMouseOptions options;
    apply_common_options(d, &options);
    if (d.contains("device_path")) {
        options.device_path = py::cast<std::string>(d["device_path"]);
    }
    if (d.contains("axis_map")) {
        assign_array6_int(py::cast<std::vector<int>>(d["axis_map"]), &options.axis_map,
                          "axis_map");
    }
    if (d.contains("axis_sign")) {
        assign_array6_int(py::cast<std::vector<int>>(d["axis_sign"]), &options.axis_sign,
                          "axis_sign");
    }
    if (d.contains("axis_scale")) {
        assign_array6_double(py::cast<std::vector<double>>(d["axis_scale"]),
                             &options.axis_scale, "axis_scale");
    }
    if (d.contains("deadzone")) {
        options.deadzone = py::cast<double>(d["deadzone"]);
    }
    return options;
}

py::dict peripheral_info_to_dict(const smrcore::peripherals::PeripheralInfo& info) {
    py::dict out;
    out["path"] = info.path;
    out["model"] = info.model;
    out["serial_number"] = info.serial_number;
    out["firmware_version"] = info.firmware_version;
    out["sample_rate_hz"] = info.sample_rate_hz;
    out["connected"] = info.connected;
    out["frame_error_count"] = info.frame_error_count;
    return out;
}

py::dict spacemouse_sample_to_dict(const smrcore::peripherals::SpaceMouseSample& sample) {
    py::dict out;
    out["x"] = sample.x;
    out["y"] = sample.y;
    out["z"] = sample.z;
    out["roll"] = sample.roll;
    out["pitch"] = sample.pitch;
    out["yaw"] = sample.yaw;
    out["gripper_command"] = static_cast<int>(sample.gripper_command);
    out["timestamp_sec"] = sample.timestamp_sec;
    return out;
}

py::dict spacemouse_raw_sample_to_dict(
    const smrcore::peripherals::SpaceMouseRawSample& sample) {
    py::dict out;
    out["axes"] = std::vector<int32_t>(sample.axes.begin(), sample.axes.end());
    out["buttons"] = sample.buttons;
    out["timestamp_sec"] = sample.timestamp_sec;
    return out;
}

py::dict ft_sensor_sample_to_dict(const smrcore::peripherals::FtSensorSample& sample) {
    py::dict out;
    out["fx"] = sample.fx;
    out["fy"] = sample.fy;
    out["fz"] = sample.fz;
    out["tx"] = sample.tx;
    out["ty"] = sample.ty;
    out["tz"] = sample.tz;
    out["timestamp_sec"] = sample.timestamp_sec;
    return out;
}

template <typename PeripheralT, typename OptionsT, OptionsT (*FromDict)(const py::dict&)>
class PyPeripheral {
public:
    bool Initialize(const py::object& options) {
        OptionsT native_options;
        if (py::isinstance<py::dict>(options)) {
            native_options = FromDict(py::cast<py::dict>(options));
        } else {
            native_options = py::cast<OptionsT>(options);
        }
        return peripheral_.Initialize(native_options);
    }

    void Shutdown() {
        peripheral_.Shutdown();
    }

    bool Start() {
        return peripheral_.Start();
    }

    void Stop() {
        peripheral_.Stop();
    }

    bool IsConnected() const {
        return peripheral_.IsConnected();
    }

    py::dict GetInfo() const {
        return peripheral_info_to_dict(peripheral_.GetInfo());
    }

protected:
    PeripheralT peripheral_;
};

class PyFtSensor : public PyPeripheral<smrcore::peripherals::FtSensor,
                                       smrcore::peripherals::FtSensorOptions,
                                       ft_sensor_options_from_dict> {
public:
    bool WaitForFirstSample() { return peripheral_.WaitForFirstSample(); }

    py::object GetSample() {
        auto sample = peripheral_.GetSample();
        if (!sample) {
            return py::none();
        }
        return ft_sensor_sample_to_dict(*sample);
    }
};

class PySpaceMouse : public PyPeripheral<smrcore::peripherals::SpaceMouse,
                                         smrcore::peripherals::SpaceMouseOptions,
                                         spacemouse_options_from_dict> {
public:
    py::object GetSample() {
        auto sample = peripheral_.GetSample();
        if (!sample) {
            return py::none();
        }
        return spacemouse_sample_to_dict(*sample);
    }

    py::object GetRawSample() {
        auto sample = peripheral_.GetRawSample();
        if (!sample) {
            return py::none();
        }
        return spacemouse_raw_sample_to_dict(*sample);
    }
};

} // namespace

PYBIND11_MODULE(_native, m) {
    m.def("build_info", [] {
        py::dict info;
        info["backend"] = "pybind11";
        return info;
    });

    m.def("linked_sdk", [] {
        py::dict info;
        info["linked"] = true;
        info["version"] = SMRCORE_PERIPHERALS_VERSION_STR;
        info["ft_sensor_type"] = "smrcore::peripherals::FtSensor";
        info["spacemouse_type"] = "smrcore::peripherals::SpaceMouse";
        return info;
    });

    py::enum_<smrcore::peripherals::GripperCommand>(m, "GripperCommand")
        .value("Open", smrcore::peripherals::GripperCommand::Open)
        .value("Close", smrcore::peripherals::GripperCommand::Close);

    py::class_<smrcore::peripherals::PeripheralOptions>(m, "PeripheralOptions")
        .def(py::init<>())
        .def_readwrite("sample_rate_hz", &smrcore::peripherals::PeripheralOptions::sample_rate_hz);

    py::class_<smrcore::peripherals::FtSensorOptions,
               smrcore::peripherals::PeripheralOptions>(m, "FtSensorOptions")
        .def(py::init<>())
        .def_readwrite("serial_port", &smrcore::peripherals::FtSensorOptions::serial_port)
        .def_readwrite("baud_rate", &smrcore::peripherals::FtSensorOptions::baud_rate)
        .def_readwrite("read_buffer_size",
                       &smrcore::peripherals::FtSensorOptions::read_buffer_size)
        .def_readwrite("sensor_type", &smrcore::peripherals::FtSensorOptions::sensor_type)
        .def_readwrite("stale_timeout_ms",
                       &smrcore::peripherals::FtSensorOptions::stale_timeout_ms);

    py::class_<smrcore::peripherals::SpaceMouseOptions,
               smrcore::peripherals::PeripheralOptions>(m, "SpaceMouseOptions")
        .def(py::init<>())
        .def_readwrite("device_path", &smrcore::peripherals::SpaceMouseOptions::device_path)
        .def_readwrite("axis_map", &smrcore::peripherals::SpaceMouseOptions::axis_map)
        .def_readwrite("axis_sign", &smrcore::peripherals::SpaceMouseOptions::axis_sign)
        .def_readwrite("axis_scale", &smrcore::peripherals::SpaceMouseOptions::axis_scale)
        .def_readwrite("deadzone", &smrcore::peripherals::SpaceMouseOptions::deadzone);

    py::class_<PyFtSensor>(m, "FtSensor")
        .def(py::init<>())
        .def("Initialize", &PyFtSensor::Initialize, py::arg("options"))
        .def("WaitForFirstSample", &PyFtSensor::WaitForFirstSample)
        .def("Shutdown", &PyFtSensor::Shutdown)
        .def("Start", &PyFtSensor::Start)
        .def("Stop", &PyFtSensor::Stop)
        .def("IsConnected", &PyFtSensor::IsConnected)
        .def("GetInfo", &PyFtSensor::GetInfo)
        .def("GetSample", &PyFtSensor::GetSample);

    py::class_<PySpaceMouse>(m, "SpaceMouse")
        .def(py::init<>())
        .def("Initialize", &PySpaceMouse::Initialize, py::arg("options"))
        .def("Shutdown", &PySpaceMouse::Shutdown)
        .def("Start", &PySpaceMouse::Start)
        .def("Stop", &PySpaceMouse::Stop)
        .def("IsConnected", &PySpaceMouse::IsConnected)
        .def("GetInfo", &PySpaceMouse::GetInfo)
        .def("GetSample", &PySpaceMouse::GetSample)
        .def("GetRawSample", &PySpaceMouse::GetRawSample);
}
