/**
 * @file serial_reader.cpp
 * @brief 力传感器通用串口读取器实现
 */

#include "protocols/ft_sensor/serial_reader.hpp"

namespace smrcore::peripherals::protocols::ft_sensor
{

SerialReader::~SerialReader() { Close(); }

bool SerialReader::Open(const SerialReaderConfig &config)
{
    Close();
    config_ = config;
    if (config_.read_buffer_size == 0)
    {
        config_.read_buffer_size = 512;
    }
    return serial_.Open(config_.serial_port, config_.baud_rate);
}

void SerialReader::Close() { serial_.Close(); }

bool SerialReader::IsOpen() const { return serial_.IsOpen(); }

} // namespace smrcore::peripherals::protocols::ft_sensor
