/**
 * @file ft_sensor.cpp
 * @brief 六维力传感器外设门面实现
 */

#include "peripherals/ft_sensor/ft_sensor.hpp"

#include "common/clock.hpp"
#include "common/latest_slot.hpp"
#include "protocols/ft_sensor/kunwei/kunwei_reader.hpp"
#include "protocols/ft_sensor/xjc/xjc_reader.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace smrcore::peripherals
{

namespace
{

constexpr int kMinFirstValidFrameTimeoutMs = 100;

double EffectiveRate(double requested)
{
    return requested > 0.0 ? requested : 1000.0;
}

enum class FtSensorBrand
{
    Kunwei,
    Xjc,
};

FtSensorBrand BrandFromType(const std::string &type)
{
    if (type == "xjc_serial" || type == "xjc")
    {
        return FtSensorBrand::Xjc;
    }
    return FtSensorBrand::Kunwei;
}

int MapXjcReportingHz(double sample_rate_hz)
{
    const int hz = static_cast<int>(sample_rate_hz);
    if (hz <= 100)
    {
        return 100;
    }
    if (hz <= 250)
    {
        return 250;
    }
    if (hz <= 500)
    {
        return 500;
    }
    return 1000;
}

bool WaitForFirstFtSensorSample(FtSensor &sensor, int stale_timeout_ms)
{
    const int timeout_ms =
        std::max(kMinFirstValidFrameTimeoutMs, 5 * stale_timeout_ms);
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (sensor.GetSample().has_value())
        {
            return true;
        }
        if (!sensor.IsConnected())
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}

} // namespace

class FtSensor::Impl
{
public:
    ~Impl() { Stop(); }

    bool Initialize(const FtSensorOptions &options)
    {
        Stop();
        std::lock_guard<std::mutex> lock(mutex_);
        options_ = options;
        options_.sample_rate_hz = EffectiveRate(options.sample_rate_hz);
        brand_ = BrandFromType(options_.sensor_type);
        info_ = {};
        info_.path = options_.serial_port;
        info_.model = brand_ == FtSensorBrand::Xjc ? "XJC F/T" : "Kunwei F/T";
        info_.sample_rate_hz = options_.sample_rate_hz;
        initialized_ = true;
        return true;
    }

    void Shutdown()
    {
        Stop();
        std::lock_guard<std::mutex> lock(mutex_);
        initialized_ = false;
        connected_ = false;
        CloseReader();
        slot_.Clear();
    }

    bool Start()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_)
        {
            return false;
        }
        if (running_.load())
        {
            return true;
        }
        if (options_.serial_port.empty())
        {
            connected_ = false;
            return false;
        }

        if (!OpenReader() || !StartStreaming())
        {
            CloseReader();
            connected_ = false;
            return false;
        }

        running_.store(true);
        connected_ = true;
        worker_ = std::thread(&Impl::RunLoop, this);
        return true;
    }

    void Stop()
    {
        running_.store(false);
        if (worker_.joinable())
        {
            worker_.join();
        }
        CloseReader();
        connected_ = false;
    }

    bool IsConnected() const { return connected_.load(); }

    PeripheralInfo GetInfo() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        PeripheralInfo info = info_;
        info.connected = connected_.load();
        return info;
    }

    std::optional<FtSensorSample> GetSample() { return slot_.Take(); }

    int GetStaleTimeoutMs() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return options_.stale_timeout_ms;
    }

private:
    bool OpenReader()
    {
        if (brand_ == FtSensorBrand::Kunwei)
        {
            protocols::ft_sensor::kunwei::KunweiReaderConfig config;
            config.serial_port = options_.serial_port;
            config.baud_rate = options_.baud_rate;
            config.read_buffer_size = options_.read_buffer_size;
            return kunwei_reader_.Open(config);
        }

        protocols::ft_sensor::xjc::XjcReaderConfig config;
        config.serial_port = options_.serial_port;
        config.baud_rate = options_.baud_rate;
        config.read_buffer_size = options_.read_buffer_size;
        config.active_reporting_hz = MapXjcReportingHz(options_.sample_rate_hz);
        return xjc_reader_.Open(config);
    }

    bool StartStreaming()
    {
        if (brand_ == FtSensorBrand::Kunwei)
        {
            return kunwei_reader_.StartStreaming();
        }
        return xjc_reader_.StartStreaming();
    }

    void CloseReader()
    {
        if (brand_ == FtSensorBrand::Kunwei)
        {
            kunwei_reader_.Close();
            return;
        }
        xjc_reader_.Close();
    }

    std::vector<FtSensorSample> ReadAvailable(int timeout_ms, std::size_t max_bytes,
                                              std::size_t max_frames,
                                              double timestamp_sec, bool *disconnected)
    {
        if (brand_ == FtSensorBrand::Kunwei)
        {
            return kunwei_reader_.ReadAvailable(timeout_ms, max_bytes, max_frames,
                                                timestamp_sec, disconnected);
        }
        return xjc_reader_.ReadAvailable(timeout_ms, max_bytes, max_frames,
                                         timestamp_sec, disconnected);
    }

    uint32_t FrameErrorCount() const
    {
        if (brand_ == FtSensorBrand::Kunwei)
        {
            return kunwei_reader_.frame_error_count();
        }
        return xjc_reader_.frame_error_count();
    }

    void RunLoop()
    {
        while (running_.load())
        {
            bool disconnected = false;
            auto samples = ReadAvailable(2, 4096, 16, internal::NowSeconds(),
                                         &disconnected);
            if (!samples.empty())
            {
                slot_.Set(samples.back());
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                info_.frame_error_count = FrameErrorCount();
            }
            if (disconnected)
            {
                connected_ = false;
                running_.store(false);
                CloseReader();
                break;
            }
            if (samples.empty())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    mutable std::mutex mutex_;
    FtSensorOptions options_;
    PeripheralInfo info_;
    FtSensorBrand brand_{FtSensorBrand::Kunwei};
    internal::LatestSlot<FtSensorSample> slot_;
    protocols::ft_sensor::kunwei::KunweiReader kunwei_reader_;
    protocols::ft_sensor::xjc::XjcReader xjc_reader_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    bool initialized_{false};
    std::thread worker_;
};

FtSensor::FtSensor() : impl_(std::make_unique<Impl>()) {}
FtSensor::~FtSensor() = default;

bool FtSensor::Initialize(const FtSensorOptions &options)
{
    return impl_->Initialize(options);
}

void FtSensor::Shutdown() { impl_->Shutdown(); }
bool FtSensor::Start() { return impl_->Start(); }
void FtSensor::Stop() { impl_->Stop(); }
bool FtSensor::IsConnected() const { return impl_->IsConnected(); }
PeripheralInfo FtSensor::GetInfo() const { return impl_->GetInfo(); }

std::optional<FtSensorSample> FtSensor::GetSample()
{
    return impl_->GetSample();
}

bool FtSensor::WaitForFirstSample()
{
    return WaitForFirstFtSensorSample(*this, impl_->GetStaleTimeoutMs());
}

} // namespace smrcore::peripherals
