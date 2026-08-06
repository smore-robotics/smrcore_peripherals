#!/usr/bin/env python3
"""Mirror of app/app_peripherals_ft_sensor_calib.cpp.

外置六维力传感器静态标定（bias / 重力 / 质心 / 安装 yaw）。
纯 SDK 客户端：MoveJ 走到多个静态姿态，经 GetFtSensorRawState 采集 raw
wrench 均值，再 Preview/SaveFtCalibration。串口读数需先由
`app_peripherals_bridge --ft-sensor`（或 Python bridge）注入控制器。

依赖：
  pip install rcore-sdk-py
"""

from __future__ import annotations

import argparse
import math
import signal
import sys
import threading
import time
from dataclasses import dataclass, field

try:
    from rcore_sdk import JointPositions, Pose, Robot
except ImportError as exc:
    raise SystemExit(
        "rcore_sdk is required for the F/T calibration app. "
        "Install the Python robot SDK wheel (rcore-sdk-py)."
    ) from exc

_POSE_ZERO_TOLERANCE = 1e-12
_POST_ENABLE_MOVEJ_DELAY_MS = 200

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


@dataclass
class AppConfig:
    robot_ip: str = ""
    settle_ms: int = 2000
    sample_ms: int = 1000
    velocity_ratio: float = 0.20
    joint0_deg: float = 0.0
    joint5_deg: float = 0.0
    save: bool = False
    sensor_in_flange_nominal: Pose = field(
        default_factory=lambda: Pose([0.0, 0.0, 0.0, 0.0, 0.0, 0.0])
    )


def calibration_points_deg(
    joint0_deg: float = 0.0, joint5_deg: float = 0.0
) -> list[list[float]]:
    points = [
        [0.0, -90.0, -90.0, -90.0, 60.0, 0.0],
        [0.0, -90.0, -90.0, -90.0, 90.0, 0.0],
        [0.0, -90.0, -90.0, -90.0, 120.0, 0.0],
        [0.0, -90.0, -90.0, -45.0, 120.0, 0.0],
        [0.0, -90.0, -90.0, -45.0, 90.0, 0.0],
        [0.0, -90.0, -90.0, -45.0, 60.0, 0.0],
        [0.0, -90.0, -90.0, -135.0, 60.0, 0.0],
        [0.0, -90.0, -90.0, -135.0, 90.0, 0.0],
        [0.0, -90.0, -90.0, -135.0, 120.0, 0.0],
    ]
    for point in points:
        point[0] = joint0_deg
        point[5] = joint5_deg
    return points


def home_point_deg(joint0_deg: float = 0.0, joint5_deg: float = 0.0) -> list[float]:
    return [joint0_deg, -90.0, -90.0, -90.0, 90.0, joint5_deg]


def to_joint_positions_deg(q_deg: list[float]) -> JointPositions:
    return JointPositions([v * math.pi / 180.0 for v in q_deg])


def parse_pose(text: str) -> Pose:
    parts = text.split(",")
    if len(parts) != 6:
        raise argparse.ArgumentTypeError(
            "--nominal expects 6 comma-separated values: x,y,z,roll,pitch,yaw"
        )
    try:
        values = [float(part) for part in parts]
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"invalid --nominal value: {text}") from exc
    return Pose.from_euler(values[0:3], values[3:6])


def parse_args(argv: list[str] | None = None) -> AppConfig:
    parser = argparse.ArgumentParser(
        description=(
            "Static external F/T sensor calibration (bias / gravity / CoM / yaw). "
            "Requires app_peripherals_bridge --ft-sensor first. "
            "reference_frame must be base; tool must be flange or a zero-offset "
            "frame under flange."
        )
    )
    parser.add_argument(
        "--robot-ip",
        default="",
        help="SDK remote robot IP (default: local DDS)",
    )
    parser.add_argument(
        "--nominal",
        type=parse_pose,
        default=None,
        help=(
            "sensor_in_flange_nominal as x,y,z,roll,pitch,yaw in m/rad "
            "(default: identity)"
        ),
    )
    parser.add_argument(
        "--settle-ms",
        type=int,
        default=2000,
        help="Static settle time after MoveJ (default: 2000)",
    )
    parser.add_argument(
        "--sample-ms",
        type=int,
        default=1000,
        help="Raw averaging window (default: 1000)",
    )
    parser.add_argument(
        "--velocity-ratio",
        type=float,
        default=0.20,
        help="MoveJ speed ratio in (0,1] (default: 0.20)",
    )
    parser.add_argument(
        "--joint0-deg",
        type=float,
        default=0.0,
        help="Joint 1 angle for all calibration and home points (default: 0)",
    )
    parser.add_argument(
        "--joint5-deg",
        type=float,
        default=0.0,
        help="Joint 6 angle for all calibration and home points (default: 0)",
    )
    parser.add_argument(
        "--save",
        action="store_true",
        help="Save passing calibration and activate it; default is Preview only",
    )
    args = parser.parse_args(argv)

    if args.settle_ms < 0 or args.sample_ms <= 0:
        parser.error("--settle-ms must be >= 0 and --sample-ms must be > 0")
    if not math.isfinite(args.velocity_ratio) or not (
        0.0 < args.velocity_ratio <= 1.0
    ):
        parser.error("--velocity-ratio must be in (0, 1]")
    if not math.isfinite(args.joint0_deg) or not math.isfinite(args.joint5_deg):
        parser.error("--joint0-deg / --joint5-deg must be finite")

    return AppConfig(
        robot_ip=args.robot_ip,
        settle_ms=args.settle_ms,
        sample_ms=args.sample_ms,
        velocity_ratio=args.velocity_ratio,
        joint0_deg=args.joint0_deg,
        joint5_deg=args.joint5_deg,
        save=bool(args.save),
        sensor_in_flange_nominal=(
            args.nominal
            if args.nominal is not None
            else Pose([0.0, 0.0, 0.0, 0.0, 0.0, 0.0])
        ),
    )


def uses_base_flange_pose_contract(controller_config: dict) -> bool:
    transform_tree = controller_config.get("transform_tree") or {}
    reference_frame = str(transform_tree.get("reference_frame", ""))
    current_tool_frame = str(transform_tree.get("current_tool_frame", ""))
    if reference_frame != "base":
        return False
    if current_tool_frame == "flange":
        return True
    for frame in transform_tree.get("nodes") or []:
        if (
            str(frame.get("name", "")) != current_tool_frame
            or str(frame.get("parent", "")) != "flange"
        ):
            continue
        xyz = list(frame.get("xyz") or [0.0, 0.0, 0.0])
        rpy = list(frame.get("rpy") or [0.0, 0.0, 0.0])
        for value in (*xyz[:3], *rpy[:3]):
            if abs(float(value)) > _POSE_ZERO_TOLERANCE:
                return False
        return True
    return False


def sleep_while_running(milliseconds: int) -> bool:
    deadline = time.monotonic() + milliseconds / 1000.0
    while _is_running() and time.monotonic() < deadline:
        time.sleep(0.01)
    return _is_running()


def wait_for_motor_ready(robot: Robot, timeout_ms: int) -> bool:
    deadline = time.monotonic() + timeout_ms / 1000.0
    while _is_running() and time.monotonic() < deadline:
        status = robot.GetMotorStatus()
        if (
            status.enabled
            and status.operational
            and not status.estop
            and not status.error
        ):
            return True
        time.sleep(0.02)
    return False


def wait_for_sample(robot: Robot, timeout_ms: int) -> bool:
    deadline = time.monotonic() + timeout_ms / 1000.0
    while _is_running() and time.monotonic() < deadline:
        state = robot.GetFtSensorRawState()
        if bool(state.get("success")) and bool(state.get("valid")):
            return True
        time.sleep(0.01)
    return False


def collect_mean(
    robot: Robot, duration_ms: int
) -> tuple[list[float], int] | tuple[None, int]:
    total = [0.0] * 6
    count = 0
    deadline = time.monotonic() + duration_ms / 1000.0
    while _is_running() and time.monotonic() < deadline:
        state = robot.GetFtSensorRawState()
        if bool(state.get("success")) and bool(state.get("valid")):
            wrench = list(state.get("wrench") or [])
            if len(wrench) >= 6:
                for i in range(6):
                    total[i] += float(wrench[i])
                count += 1
        time.sleep(0.01)
    if count == 0 or not _is_running():
        return None, count
    return [value / count for value in total], count


def print_calibration_result(result: dict) -> None:
    params = result.get("params") or {}
    force_bias = list(params.get("force_bias") or [0.0, 0.0, 0.0])
    torque_bias = list(params.get("torque_bias") or [0.0, 0.0, 0.0])
    com = list(params.get("com_in_sensor_m") or [0.0, 0.0, 0.0])
    print(
        f"force_bias [N]: {force_bias[0]}, {force_bias[1]}, {force_bias[2]}",
        flush=True,
    )
    print(
        f"torque_bias [Nm]: {torque_bias[0]}, {torque_bias[1]}, {torque_bias[2]}",
        flush=True,
    )
    print(f"gravity_n [N]: {params.get('gravity_n', 0.0)}", flush=True)
    print(
        f"com_in_sensor_m [m]: {com[0]}, {com[1]}, {com[2]}",
        flush=True,
    )
    print(
        f"yaw_correction_rad: {params.get('yaw_correction_rad', 0.0)}",
        flush=True,
    )
    print(
        f"force_rms_n: {result.get('force_rms_n', 0.0)}, "
        f"torque_rms_nm: {result.get('torque_rms_nm', 0.0)}",
        flush=True,
    )


def run(config: AppConfig) -> int:
    robot = Robot()
    if not robot.Initialize(config.robot_ip):
        print("Robot SDK initialize failed.", file=sys.stderr, flush=True)
        return 1

    controller_config = robot.GetControllerConfig()
    if not uses_base_flange_pose_contract(controller_config):
        transform_tree = controller_config.get("transform_tree") or {}
        print(
            "Calibration requires reference_frame=base and a flange "
            "or zero-offset flange tool frame; current selection is "
            f"{transform_tree.get('reference_frame')}/"
            f"{transform_tree.get('current_tool_frame')}.",
            file=sys.stderr,
            flush=True,
        )
        robot.Shutdown()
        return 1

    initial_motor_status = robot.GetMotorStatus()
    initially_enabled = bool(initial_motor_status.enabled)
    enabled_by_app = False
    sensor_started = False
    moved = False

    def cleanup() -> None:
        nonlocal enabled_by_app, sensor_started, moved
        if moved:
            print("Returning to configured home point...", flush=True)
            home_result = robot.MoveJ(
                to_joint_positions_deg(
                    home_point_deg(config.joint0_deg, config.joint5_deg)
                ),
                asynchronous=False,
            )
            if not home_result.success:
                print(
                    f"Return home failed: {home_result.error_msg}",
                    file=sys.stderr,
                    flush=True,
                )
        if sensor_started:
            release = robot.ReleaseFtSensor()
            if not release.success:
                print(
                    f"ReleaseFtSensor failed: {release.error_msg}",
                    file=sys.stderr,
                    flush=True,
                )
        if enabled_by_app:
            robot.Disable()
        robot.Shutdown()

    sensor_result = robot.EnsureFtSensor()
    if not sensor_result.success:
        print(
            f"EnsureFtSensor failed ({sensor_result.error_code}): "
            f"{sensor_result.error_msg}",
            file=sys.stderr,
            flush=True,
        )
        cleanup()
        return 1
    sensor_started = True

    if not wait_for_sample(robot, 1500):
        print(
            "No valid F/T raw sample after EnsureFtSensor; start "
            "app_peripherals_bridge --ft-sensor first.",
            file=sys.stderr,
            flush=True,
        )
        cleanup()
        return 1

    if not initially_enabled:
        print("Enabling robot motors...", flush=True)
        enable = robot.Enable()
        if not enable.success:
            print(
                f"Enable failed: {enable.error_msg}",
                file=sys.stderr,
                flush=True,
            )
            cleanup()
            return 1
        enabled_by_app = True

    if not wait_for_motor_ready(robot, 3000):
        status = robot.GetMotorStatus()
        print(
            "Motors not ready for calibration MoveJ: "
            f"enabled={status.enabled}, operational={status.operational}, "
            f"estop={status.estop}, error={status.error}.",
            file=sys.stderr,
            flush=True,
        )
        cleanup()
        return 1

    if not sleep_while_running(_POST_ENABLE_MOVEJ_DELAY_MS):
        print(
            "Calibration interrupted before first MoveJ.",
            file=sys.stderr,
            flush=True,
        )
        cleanup()
        return 1

    speed = robot.SetVelocityPercentage([config.velocity_ratio] * 6)
    if not speed.success:
        print(
            f"SetVelocityPercentage failed: {speed.error_msg}",
            file=sys.stderr,
            flush=True,
        )
        cleanup()
        return 1

    points = calibration_points_deg(config.joint0_deg, config.joint5_deg)
    samples: list[dict] = []
    mode = "save" if config.save else "preview"
    print(
        f"Collecting {len(points)} static samples; mode={mode}.",
        flush=True,
    )

    for index, point in enumerate(points):
        if not _is_running():
            break
        print(f"Point {index + 1}/{len(points)}: MoveJ", flush=True)
        move = robot.MoveJ(to_joint_positions_deg(point), asynchronous=False)
        if not move.success:
            print(
                f"MoveJ failed: {move.error_msg}",
                file=sys.stderr,
                flush=True,
            )
            cleanup()
            return 1
        moved = True
        if not sleep_while_running(config.settle_ms):
            break

        state = robot.GetState()
        fk_result, flange_in_base = robot.ForwardKinematics(state.positions)
        if not fk_result.success:
            print(
                f"ForwardKinematics failed: {fk_result.error_msg}",
                file=sys.stderr,
                flush=True,
            )
            cleanup()
            return 1

        mean, sample_count = collect_mean(robot, config.sample_ms)
        if mean is None:
            print(
                "Failed to acquire static raw wrench window.",
                file=sys.stderr,
                flush=True,
            )
            cleanup()
            return 1

        print(
            f"  raw samples: {sample_count}, mean force [N]: "
            f"{mean[0]}, {mean[1]}, {mean[2]}",
            flush=True,
        )
        samples.append(
            {
                "id": f"pose-{index + 1}",
                "flange_in_base": flange_in_base.to_list(),
                "raw_wrench": mean,
            }
        )

    if not _is_running():
        print("Calibration interrupted.", file=sys.stderr, flush=True)
        cleanup()
        return 1

    request = {
        "sensor_in_flange_nominal": config.sensor_in_flange_nominal.to_list(),
        "samples": samples,
    }
    preview = robot.PreviewFtCalibration(request)
    preview_status = preview.get("status") or {}
    if not bool(preview_status.get("success")):
        print(
            "PreviewFtSensorCalibration failed ("
            f"{preview_status.get('error_code')}): "
            f"{preview_status.get('error_msg')}",
            file=sys.stderr,
            flush=True,
        )
        cleanup()
        return 1

    print("Preview result:", flush=True)
    print_calibration_result(preview)

    if config.save:
        saved = robot.SaveFtCalibration(request)
        saved_status = saved.get("status") or {}
        if not bool(saved_status.get("success")):
            print(
                "SaveFtSensorCalibration failed ("
                f"{saved_status.get('error_code')}): "
                f"{saved_status.get('error_msg')}",
                file=sys.stderr,
                flush=True,
            )
            cleanup()
            return 1
        print("Saved and activated calibration parameters.", flush=True)
        print_calibration_result(saved)

    cleanup()
    return 0


def main(argv: list[str] | None = None) -> int:
    signal.signal(signal.SIGINT, _handle_signal)
    signal.signal(signal.SIGTERM, _handle_signal)
    config = parse_args(argv)
    return run(config)


if __name__ == "__main__":
    raise SystemExit(main())
