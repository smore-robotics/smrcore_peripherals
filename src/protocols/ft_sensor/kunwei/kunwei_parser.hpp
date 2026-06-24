/**
 * @file kunwei_parser.hpp
 * @brief 坤维（Kunwei）六维力传感器串口帧解析
 *
 * 协议：固定 28 字节帧
 * - 帧头 `0x48 0xaa`
 * - 6× float32 LE（Fx,Fy,Fz,Tx,Ty,Tz），单位 kg / kg·m，解析后乘 g 转为 N / N·m
 * - 帧尾 `0x0d 0x0a`
 *
 * @author Smartmore Corporation
 * @date 2026-06-18
 */

#pragma once

#include "peripherals/types.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace smrcore::peripherals::protocols::ft_sensor::kunwei
{

/**
 * @class KunweiParser
 * @brief 坤维 V1 固定帧流式解析器
 *
 * 内部维护滑动缓冲区，对不完整帧等待后续字节；
 * 帧同步失败时递增 `frame_error_count()` 并尝试重新对齐帧头。
 */
class KunweiParser
{
public:
    static constexpr std::size_t kFrameSize = 28;

    /**
     * @brief 追加原始字节并返回本次解析出的全部完整帧
     * @param data 新到达的字节
     * @param size 字节数
     * @param timestamp_sec 写入每个样本的时间戳
     */
    std::vector<FtSensorSample> Feed(const uint8_t *data, std::size_t size,
                                     double timestamp_sec);

    /**
     * @brief 喂入数据后仅返回最后一帧（便于单样本 consume 语义）
     */
    std::optional<FtSensorSample> DrainLatest(const uint8_t *data,
                                              std::size_t size,
                                              double timestamp_sec);

    /** @return 自上次 `Reset()` 以来检测到的帧同步/校验错误次数 */
    uint32_t frame_error_count() const { return frame_error_count_; }

    /** @brief 清空内部缓冲与错误计数 */
    void Reset();

private:
    static bool IsFooter(const std::vector<uint8_t> &buffer, std::size_t offset);
    static float ReadFloatLe(const uint8_t *data);

    std::vector<uint8_t> buffer_;
    uint32_t frame_error_count_{0};
};

} // namespace smrcore::peripherals::protocols::ft_sensor::kunwei
