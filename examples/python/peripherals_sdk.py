#!/usr/bin/env python3
"""SpaceMouse + F/T samples -> robot external input (mirrors peripherals_sdk.cpp).

FDCC 柔顺控制需要同时向机器人提供 SpaceMouse 遥操作输入与外部六维力/力矩
传感器数据。本脚本在一个进程内只创建一个 `rcore_sdk.Robot` 会话，默认同时
启动两路外设 reader，并通过 `robot.Peripheral().Update*Sample()` 把采样写入
rcore 控制器使用的 DDS topic。

设备路径、串口、波特率等参数在下方「设备配置」区按现场修改；命令行仅保留
机器人连接与「只开一路外设」的开关。

依赖：
  pip install rcore-peripherals-py rcore-sdk-py
"""

from __future__ import annotations

import argparse
import signal
import sys
import threading
import time
from dataclasses import dataclass

try:
    from rcore_sdk import (
        FtSensorSample as SdkFtSensorSample,
        Robot,
        SpaceMouseSample as SdkSpaceMouseSample,
    )
except ImportError as exc:
    raise SystemExit(
        "rcore_sdk is required. Install the Python wheel matching .sdk-version, e.g.\n"
        "  pip install rcore-sdk-py==$(cat ../../.sdk-version)"
    ) from exc

from rcore_peripherals import FtSensor, FtSensorOptions, SpaceMouse, SpaceMouseOptions

# ============================================================================
# 设备配置（按现场硬件修改；本示例不提供对应命令行参数）
# ============================================================================

# SpaceMouse 的 Linux input 设备路径，例如 `/dev/input/event7`；空字符串表示自动探测。
K_SPACEMOUSE_DEVICE_PATH = ""

# SpaceMouse 读取/输出采样率 [Hz]；<= 0 时使用外设库默认值（125 Hz）。
K_SPACEMOUSE_SAMPLE_RATE_HZ = 0.0

# 六维力传感器串口设备路径，例如 `/dev/ttyUSB0`。
K_FT_SENSOR_SERIAL_PORT = "/dev/ttyUSB0"

# 力传感器串口波特率 [bit/s]；坤维常用 460800。
K_FT_SENSOR_BAUD_RATE = 460800

# 力传感器协议：`xjc_serial`（鑫精诚）、`kunwei_serial`（坤维）。
K_FT_SENSOR_TYPE = "xjc_serial"

# 力传感器读取/输出采样率 [Hz]；<= 0 时使用外设库默认值（1000 Hz）。
K_FT_SENSOR_SAMPLE_RATE_HZ = 1000.0

# 力传感器首帧等待的基础超时 [ms]。
K_FT_SENSOR_STALE_TIMEOUT_MS = 20

# 桥接线程轮询休眠 [s]。
K_SPACEMOUSE_POLL_SLEEP_S = 0.008
K_FT_SENSOR_POLL_SLEEP_S = 0.001

# ============================================================================
# 进程级状态
# ============================================================================

_running = True
_running_lock = threading.Lock()


def _set_running(value: bool) -> None:
    global _running
    with _running_lock:
        _running = value


def _is_running() -> bool:
    with _running_lock:
        return _running


def _handle_signal(*_args: object) -> None:
    _set_running(False)


def _to_sdk_spacemouse(sample: object) -> SdkSpaceMouseSample:
    return SdkSpaceMouseSample(
        x=sample.x,
        y=sample.y,
        z=sample.z,
        roll=sample.roll,
        pitch=sample.pitch,
        yaw=sample.yaw,
        gripper_command=sample.gripper_command,
        timestamp_sec=sample.timestamp_sec,
    )


def _to_sdk_ft_sensor(sample: object) -> SdkFtSensorSample:
    return SdkFtSensorSample(
        fx=sample.fx,
        fy=sample.fy,
        fz=sample.fz,
        tx=sample.tx,
        ty=sample.ty,
        tz=sample.tz,
        timestamp_sec=sample.timestamp_sec,
    )


def _run_spacemouse_bridge(
    spacemouse: SpaceMouse,
    peripheral: object,
    sdk_lock: threading.Lock,
) -> None:
    while _is_running():
        sample = spacemouse.GetSample()
        if sample is not None:
            with sdk_lock:
                result = peripheral.UpdateSpaceMouseSample(_to_sdk_spacemouse(sample))
            if not result.success:
                print(
                    "UpdateSpaceMouseSample failed:",
                    result.error_code,
                    result.error_msg,
                    file=sys.stderr,
                    flush=True,
                )
        time.sleep(K_SPACEMOUSE_POLL_SLEEP_S)


def _run_ft_sensor_bridge(
    sensor: FtSensor,
    peripheral: object,
    sdk_lock: threading.Lock,
) -> None:
    while _is_running():
        sample = sensor.GetSample()
        if sample is not None:
            with sdk_lock:
                result = peripheral.UpdateFtSensorSample(_to_sdk_ft_sensor(sample))
            if not result.success:
                print(
                    "UpdateFtSensorSample failed:",
                    result.error_code,
                    result.error_msg,
                    file=sys.stderr,
                    flush=True,
                )
        time.sleep(K_FT_SENSOR_POLL_SLEEP_S)


@dataclass
class ExampleOptions:
    robot_ip: str = ""
    spacemouse_enabled: bool = True
    ft_sensor_enabled: bool = True


def _build_spacemouse_options() -> SpaceMouseOptions:
    return SpaceMouseOptions(
        sample_rate_hz=K_SPACEMOUSE_SAMPLE_RATE_HZ,
        device_path=K_SPACEMOUSE_DEVICE_PATH,
    )


def _build_ft_sensor_options() -> FtSensorOptions:
    return FtSensorOptions(
        serial_port=K_FT_SENSOR_SERIAL_PORT,
        baud_rate=K_FT_SENSOR_BAUD_RATE,
        sensor_type=K_FT_SENSOR_TYPE,
        sample_rate_hz=K_FT_SENSOR_SAMPLE_RATE_HZ,
        stale_timeout_ms=K_FT_SENSOR_STALE_TIMEOUT_MS,
    )


def parse_args(argv: list[str] | None = None) -> ExampleOptions:
    parser = argparse.ArgumentParser(
        description="Stream SpaceMouse and/or F/T sensor samples into robot SDK",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Selection rule: omit --spacemouse/--ft-sensor to enable both; "
            "pass either flag to enable only the selected peripheral(s).\n"
            "Device paths and serial settings are constants at the top of this file."
        ),
    )
    parser.add_argument("--robot", default="", help="Robot controller IP (optional)")
    parser.add_argument(
        "--spacemouse",
        action="store_true",
        help="Enable SpaceMouse (with other flags, selects peripherals explicitly)",
    )
    parser.add_argument(
        "--ft-sensor",
        action="store_true",
        help="Enable F/T sensor (with other flags, selects peripherals explicitly)",
    )
    args = parser.parse_args(argv)

    options = ExampleOptions(robot_ip=args.robot)
    if args.spacemouse or args.ft_sensor:
        options.spacemouse_enabled = bool(args.spacemouse)
        options.ft_sensor_enabled = bool(args.ft_sensor)
    return options


def main(argv: list[str] | None = None) -> int:
    options = parse_args(argv)

    signal.signal(signal.SIGINT, _handle_signal)
    signal.signal(signal.SIGTERM, _handle_signal)

    robot = Robot()
    if not robot.Initialize(options.robot_ip):
        print("failed to initialize robot SDK", file=sys.stderr, flush=True)
        return 1

    spacemouse = SpaceMouse()
    if options.spacemouse_enabled:
        if not spacemouse.Initialize(_build_spacemouse_options()) or not spacemouse.Start():
            print("failed to start SpaceMouse reader", file=sys.stderr, flush=True)
            return 1

    sensor = FtSensor()
    if options.ft_sensor_enabled:
        if not sensor.Initialize(_build_ft_sensor_options()) or not sensor.Start():
            print("failed to start F/T sensor reader", file=sys.stderr, flush=True)
            return 1
        if not sensor.WaitForFirstSample():
            info = sensor.GetInfo()
            print(
                "first valid F/T frame timeout, frame_errors="
                f"{info.frame_error_count}",
                file=sys.stderr,
                flush=True,
            )
            return 1

    peripheral = robot.Peripheral()
    sdk_lock = threading.Lock()
    threads: list[threading.Thread] = []

    if options.spacemouse_enabled:
        thread = threading.Thread(
            target=_run_spacemouse_bridge,
            args=(spacemouse, peripheral, sdk_lock),
            name="spacemouse-bridge",
        )
        thread.start()
        threads.append(thread)

    if options.ft_sensor_enabled:
        thread = threading.Thread(
            target=_run_ft_sensor_bridge,
            args=(sensor, peripheral, sdk_lock),
            name="ft-sensor-bridge",
        )
        thread.start()
        threads.append(thread)

    try:
        while _is_running():
            time.sleep(0.1)
    finally:
        _set_running(False)
        for thread in threads:
            thread.join()
        robot.Shutdown()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
