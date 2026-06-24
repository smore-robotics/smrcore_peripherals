/**
 * @file spacemouse.cpp
 * @brief SpaceMouse 外设门面实现
 */

#include "peripherals/spacemouse/spacemouse.hpp"

#include "common/clock.hpp"
#include "peripherals/spacemouse/sample_model.hpp"
#include "protocols/spacemouse/linux_input_reader.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace smrcore::peripherals
{

namespace
{

double EffectiveRate(double requested)
{
    return requested > 0.0 ? requested : 125.0;
}

bool HasAxisInput(const std::array<int32_t, 6> &axes)
{
    return std::any_of(axes.begin(), axes.end(),
                       [](int32_t value) { return value != 0; });
}

constexpr double kIdleTimeoutSeconds = 0.04;

} // namespace

class SpaceMouse::Impl
{
public:
    ~Impl() { Stop(); }

    bool Initialize(const SpaceMouseOptions &options)
    {
        Stop();
        std::lock_guard<std::mutex> lock(mutex_);
        options_ = options;
        options_.sample_rate_hz = EffectiveRate(options.sample_rate_hz);
        model_.Configure(options_);
        info_ = {};
        info_.path = options_.device_path;
        info_.model = "SpaceMouse";
        info_.sample_rate_hz = options_.sample_rate_hz;
        if (info_.path.empty())
        {
            const auto devices = protocols::spacemouse::LinuxInputReader::ScanDevices();
            if (!devices.empty())
            {
                info_.path = devices.front().event_path;
                info_.model = devices.front().name;
            }
        }
        initialized_ = true;
        return true;
    }

    void Shutdown()
    {
        Stop();
        std::lock_guard<std::mutex> lock(mutex_);
        initialized_ = false;
        connected_ = false;
        reader_.Close();
        model_.Reset();
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
        std::string path = options_.device_path;
        if (path.empty())
        {
            const auto devices = protocols::spacemouse::LinuxInputReader::ScanDevices();
            if (!devices.empty())
            {
                path = devices.front().event_path;
                info_.model = devices.front().name;
            }
        }
        if (path.empty() || !reader_.Open(path))
        {
            connected_ = false;
            return false;
        }
        info_.path = path;
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
        reader_.Close();
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

    std::optional<SpaceMouseSample> GetSample() { return model_.TakeSample(); }

    std::optional<SpaceMouseRawSample> GetRawSample()
    {
        return model_.TakeRawSample();
    }

private:
    void RunLoop()
    {
        const double period_sec = 1.0 / std::max(1.0, options_.sample_rate_hz);
        const auto period =
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(period_sec));
        auto next_tick = std::chrono::steady_clock::now();
        uint32_t last_buttons = 0;
        double last_axis_input_sec = 0.0;

        while (running_.load())
        {
            protocols::spacemouse::RawInputFrame frame;
            bool disconnected = false;
            // 非阻塞排空 Linux input 队列，更新原始状态
            while (reader_.ReadFrame(frame, 0, &disconnected))
            {
                last_buttons = frame.buttons;
                if (HasAxisInput(frame.axes))
                {
                    last_axis_input_sec = frame.timestamp_sec;
                }
                model_.ApplyRawState(frame.axes, frame.buttons,
                                     frame.timestamp_sec);
            }
            if (disconnected)
            {
                connected_ = false;
                running_.store(false);
                reader_.Close();
                break;
            }

            const double now_sec = internal::NowSeconds();
            // 轴输入空闲超时后清零平移/旋转，避免漂移
            if (last_axis_input_sec > 0.0 &&
                now_sec - last_axis_input_sec >= kIdleTimeoutSeconds)
            {
                model_.ApplyRawState({{0, 0, 0, 0, 0, 0}}, last_buttons,
                                     now_sec);
                last_axis_input_sec = 0.0;
            }

            const auto now = std::chrono::steady_clock::now();
            if (now >= next_tick)
            {
                model_.Tick(now_sec);
                next_tick += period;
                if (now > next_tick + period)
                {
                    next_tick = now + period;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    mutable std::mutex mutex_;
    SpaceMouseOptions options_;
    PeripheralInfo info_;
    internal::SampleModel model_;
    protocols::spacemouse::LinuxInputReader reader_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    bool initialized_{false};
    std::thread worker_;
};

SpaceMouse::SpaceMouse() : impl_(std::make_unique<Impl>()) {}
SpaceMouse::~SpaceMouse() = default;

bool SpaceMouse::Initialize(const SpaceMouseOptions &options)
{
    return impl_->Initialize(options);
}

void SpaceMouse::Shutdown() { impl_->Shutdown(); }
bool SpaceMouse::Start() { return impl_->Start(); }
void SpaceMouse::Stop() { impl_->Stop(); }
bool SpaceMouse::IsConnected() const { return impl_->IsConnected(); }
PeripheralInfo SpaceMouse::GetInfo() const { return impl_->GetInfo(); }

std::optional<SpaceMouseSample> SpaceMouse::GetSample()
{
    return impl_->GetSample();
}

std::optional<SpaceMouseRawSample> SpaceMouse::GetRawSample()
{
    return impl_->GetRawSample();
}

} // namespace smrcore::peripherals
