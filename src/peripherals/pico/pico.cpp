/**
 * @file pico.cpp
 * @brief PICO WebXR 外设门面实现
 */

#include "peripherals/pico/pico.hpp"

#include "protocols/pico/webxr_server.hpp"

#include <mutex>
#include <sstream>

namespace smrcore::peripherals
{

class Pico::Impl
{
public:
    bool Initialize(const PicoOptions &options)
    {
        Stop();
        std::lock_guard<std::mutex> lock(mutex_);
        if (options.port == 0)
        {
            last_error_ = "port must be in range 1..65535";
            initialized_ = false;
            return false;
        }
        options_ = options;
        options_.sample_rate_hz =
            options.sample_rate_hz > 0.0 ? options.sample_rate_hz : 50.0;
        info_ = {};
        std::ostringstream endpoint;
        endpoint << "http://127.0.0.1:" << options_.port << '/';
        info_.path = endpoint.str();
        info_.model = "PICO WebXR";
        info_.sample_rate_hz = options_.sample_rate_hz;
        last_error_.clear();
        initialized_ = true;
        return true;
    }

    void Shutdown()
    {
        Stop();
        std::lock_guard<std::mutex> lock(mutex_);
        initialized_ = false;
    }

    bool Start()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_)
        {
            last_error_ = "Pico is not initialized";
            return false;
        }
        if (server_.IsRunning())
        {
            return true;
        }
        last_error_.clear();
        return server_.Start(options_.port, last_error_);
    }

    void Stop() { server_.Stop(); }

    bool IsConnected() const { return server_.IsClientConnected(); }

    PeripheralInfo GetInfo() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        PeripheralInfo info = info_;
        info.connected = server_.IsClientConnected();
        return info;
    }

    std::optional<PicoSample> GetSample() { return server_.TakeSample(); }

    std::string GetLastError() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_error_;
    }

private:
    mutable std::mutex mutex_;
    PicoOptions options_;
    PeripheralInfo info_;
    protocols::pico::WebXrServer server_;
    std::string last_error_;
    bool initialized_{false};
};

Pico::Pico() : impl_(std::make_unique<Impl>()) {}
Pico::~Pico() = default;

bool Pico::Initialize(const PicoOptions &options)
{
    return impl_->Initialize(options);
}

void Pico::Shutdown() { impl_->Shutdown(); }
bool Pico::Start() { return impl_->Start(); }
void Pico::Stop() { impl_->Stop(); }
bool Pico::IsConnected() const { return impl_->IsConnected(); }
PeripheralInfo Pico::GetInfo() const { return impl_->GetInfo(); }
std::optional<PicoSample> Pico::GetSample() { return impl_->GetSample(); }
std::string Pico::GetLastError() const { return impl_->GetLastError(); }

} // namespace smrcore::peripherals
