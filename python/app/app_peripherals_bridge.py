#!/usr/bin/env python3
"""Mirror of app/app_peripherals_bridge.cpp.

单一机器人 SDK 会话内按 CLI 启动 SpaceMouse 和/或 F/T reader，并通过
`robot.Peripheral().Update*Sample()` 将采样送入机器人控制器。

依赖（需分别安装 wheel）：
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
        "rcore_sdk is required for the bridge app. Install the Python robot SDK wheel."
    ) from exc

from rcore_peripherals import FtSensor, FtSensorOptions, SpaceMouse, SpaceMouseOptions

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
        time.sleep(0.008)


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
        time.sleep(0.001)


@dataclass
class BridgeOptions:
    robot_ip: str = ""
    spacemouse_enabled: bool = True
    ft_sensor_enabled: bool = True
    spacemouse: SpaceMouseOptions | None = None
    ft_sensor: FtSensorOptions | None = None

    def __post_init__(self) -> None:
        if self.spacemouse is None:
            self.spacemouse = SpaceMouseOptions()
        if self.ft_sensor is None:
            self.ft_sensor = FtSensorOptions(
                serial_port="/dev/ttyUSB0",
                sample_rate_hz=1000.0,
            )


def parse_args(argv: list[str] | None = None) -> BridgeOptions:
    parser = argparse.ArgumentParser(
        description="Bridge SpaceMouse and/or F/T sensor samples into robot SDK"
    )
    parser.add_argument("--robot-ip", default="", help="Robot controller IP (optional)")
    parser.add_argument("--spacemouse", action="store_true", help="Enable SpaceMouse")
    parser.add_argument("--ft-sensor", action="store_true", help="Enable F/T sensor")
    parser.add_argument("--spacemouse-device", default="", help="SpaceMouse device path")
    parser.add_argument(
        "--spacemouse-sample-rate",
        type=float,
        default=0.0,
        help="SpaceMouse sample rate [Hz]",
    )
    parser.add_argument("--ft-serial-port", default="/dev/ttyUSB0")
    parser.add_argument("--ft-sensor-type", default="xjc_serial")
    parser.add_argument("--ft-baud-rate", type=int, default=460800)
    parser.add_argument("--ft-sample-rate", type=float, default=1000.0)
    parser.add_argument("--ft-stale-timeout-ms", type=int, default=20)
    args = parser.parse_args(argv)

    options = BridgeOptions(
        robot_ip=args.robot_ip,
        spacemouse=SpaceMouseOptions(
            device_path=args.spacemouse_device,
            sample_rate_hz=args.spacemouse_sample_rate,
        ),
        ft_sensor=FtSensorOptions(
            serial_port=args.ft_serial_port,
            sensor_type=args.ft_sensor_type,
            baud_rate=args.ft_baud_rate,
            sample_rate_hz=args.ft_sample_rate,
            stale_timeout_ms=args.ft_stale_timeout_ms,
        ),
    )
    if args.spacemouse or args.ft_sensor:
        options.spacemouse_enabled = bool(args.spacemouse)
        options.ft_sensor_enabled = bool(args.ft_sensor)
    return options


def main(argv: list[str] | None = None) -> int:
    options = parse_args(argv)

    signal.signal(signal.SIGINT, _handle_signal)
    signal.signal(signal.SIGTERM, _handle_signal)

    robot = Robot()
    ip_suffix = f" {options.robot_ip}" if options.robot_ip else ""
    print(f"正在连接机器人{ip_suffix} ...", flush=True)
    if not robot.Initialize(options.robot_ip):
        print("failed to initialize robot SDK", file=sys.stderr, flush=True)
        return 1
    print("机器人 SDK 连接成功", flush=True)

    spacemouse = SpaceMouse()
    if options.spacemouse_enabled:
        assert options.spacemouse is not None
        print("正在启动 SpaceMouse ...", flush=True)
        if not spacemouse.Initialize(options.spacemouse) or not spacemouse.Start():
            print("failed to start SpaceMouse reader", file=sys.stderr, flush=True)
            return 1
        print("SpaceMouse 已启动", flush=True)

    sensor = FtSensor()
    if options.ft_sensor_enabled:
        assert options.ft_sensor is not None
        print(f"正在启动力传感器 {options.ft_sensor.serial_port} ...", flush=True)
        if not sensor.Initialize(options.ft_sensor) or not sensor.Start():
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
        print("力传感器已启动", flush=True)

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

    channels = ""
    if options.spacemouse_enabled:
        channels += " [SpaceMouse]"
    if options.ft_sensor_enabled:
        channels += " [F/T]"
    print(f"连接成功，正在发送外设消息{channels}（Ctrl+C 退出）", flush=True)

    try:
        while _is_running():
            time.sleep(0.1)
    finally:
        _set_running(False)
        for thread in threads:
            thread.join()
        robot.Shutdown()
        print("外设 bridge 已停止", flush=True)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
