#!/usr/bin/env python3
"""Direct Zhonglin master arm -> single-arm i3 ServoJ teleoperation (no ROS)."""

from __future__ import annotations

import argparse
import math
import signal
import time

import numpy as np

from rcore_peripherals.zhonglin import (
    ALIGN_TOLERANCE_DEG,
    HOME_PWM,
    SIGNS,
    ZhonglinReader,
    deg,
    master_pwm_to_i3_joints,
    max_joint_error_deg,
    result_ok,
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true", help="Confirm real single-arm i3 control")
    parser.add_argument("--stale-timeout", type=float, default=0.5)
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--robot-ip", required=True)
    parser.add_argument("--rate", type=float, default=20.0)
    parser.add_argument("--max-offset-deg", type=float, default=0.0)
    parser.add_argument(
        "--align-start",
        action="store_true",
        help="Move i3 to the master's current mapped pose before streaming",
    )
    parser.add_argument(
        "--align-velocity-scale",
        type=float,
        default=0.1,
        help="MoveJ speed scale used by --align-start (default 0.1)",
    )
    parser.add_argument(
        "--align-tolerance-deg",
        type=float,
        default=ALIGN_TOLERANCE_DEG,
        help="Skip startup MoveJ when already within this error",
    )
    parser.add_argument(
        "--max-step-deg",
        type=float,
        default=0.0,
        help="Max joint step per teleop frame; 0 disables script-side filtering",
    )
    parser.add_argument("--keep-master-torque", action="store_true")
    args = parser.parse_args()

    if not 5.0 <= args.rate <= 50.0:
        parser.error("--rate must be between 5 and 50 Hz")
    if args.max_offset_deg < 0.0 or args.max_offset_deg > 180.0:
        parser.error("--max-offset-deg must be 0 (use joint limits) or in (0, 180]")
    if 0.0 < args.max_offset_deg < 5.0:
        parser.error("--max-offset-deg safety cap must be at least 5 degrees")
    if args.max_step_deg < 0.0 or args.max_step_deg > 30.0:
        parser.error("--max-step-deg must be 0 (disabled) or between 1 and 30 degrees")
    if not 0.0 < args.align_velocity_scale <= 1.0:
        parser.error("--align-velocity-scale must be in (0, 1]")
    if args.align_tolerance_deg < 0.0:
        parser.error("--align-tolerance-deg must be non-negative")

    if not args.execute:
        parser.error("--execute is required; use app_peripherals_read_zhonglin.py for read-only data")
    if not math.isfinite(args.stale_timeout) or not 0.1 <= args.stale_timeout <= 2.0:
        parser.error("--stale-timeout must be 0.1..2 seconds")
    for value in (args.rate, args.max_offset_deg, args.max_step_deg, args.align_velocity_scale, args.align_tolerance_deg):
        if not math.isfinite(value):
            parser.error("all numeric options must be finite")
    import rcore_sdk
    running = True

    def stop(_signum: int, _frame: object) -> None:
        nonlocal running
        running = False

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    reader = ZhonglinReader(args.port, args.baudrate)
    robot = None
    connected = False
    try:
        robot = rcore_sdk.Robot()
        initial_pwm = reader.read_all()
        if initial_pwm is None:
            raise RuntimeError("cannot read all Zhonglin servos 0-5")

        if not robot.Initialize(args.robot_ip):
            raise RuntimeError("rcore_sdk.Robot.Initialize failed")
        connected = True
        state = robot.GetState()
        robot_q = np.asarray(list(state.positions), dtype=float)
        if robot_q.size != 6 or not np.all(np.isfinite(robot_q)):
            raise RuntimeError(f"invalid i3 state: {robot_q}")

        nominal_q = master_pwm_to_i3_joints(
            initial_pwm, max_offset_deg=args.max_offset_deg
        )
        nominal_err = max_joint_error_deg(robot_q, nominal_q)
        if nominal_err > ALIGN_TOLERANCE_DEG:
            print(
                f"Note: i3 differs from nominal HOME mapping by {nominal_err:.1f} deg "
                f"(actual={deg(robot_q)} nominal={deg(nominal_q)})",
                flush=True,
            )

        master_pwm_to_i3_joints(initial_pwm, reference_pwm=initial_pwm, reference_q=robot_q)
        if not result_ok(robot.Enable()):
            raise RuntimeError("i3 Enable failed")

        if args.align_start:
            if nominal_err <= args.align_tolerance_deg:
                print(
                    f"Startup alignment already within {args.align_tolerance_deg:.1f} deg; "
                    "skipping MoveJ.",
                    flush=True,
                )
            else:
                print(
                    f"Aligning i3 to master's current pose (error={nominal_err:.1f} deg, "
                    f"velocity_scale={args.align_velocity_scale:.2f})...",
                    flush=True,
                )
                move = robot.MoveJ(
                    rcore_sdk.JointPositions(nominal_q.tolist()),
                    asynchronous=False,
                    velocity_scale=args.align_velocity_scale,
                )
                if not result_ok(move):
                    raise RuntimeError(
                        f"startup alignment MoveJ failed: "
                        f"{getattr(move, 'error_msg', move)}"
                    )
                state = robot.GetState()
                robot_q = np.asarray(list(state.positions), dtype=float)
                if robot_q.size != 6 or not np.all(np.isfinite(robot_q)):
                    raise RuntimeError(f"invalid i3 state after alignment: {robot_q}")
                aligned_err = max_joint_error_deg(robot_q, nominal_q)
                print(
                    f"Startup alignment complete: i3_deg={deg(robot_q)} "
                    f"error_deg={aligned_err:.2f}",
                    flush=True,
                )
                if aligned_err > args.align_tolerance_deg:
                    raise RuntimeError(
                        f"startup alignment incomplete: error={aligned_err:.2f} deg"
                    )

        if not args.keep_master_torque:
            reader.release_torque()

        print("Zhonglin -> i3 teleoperation started", flush=True)
        print(f"home_pwm={HOME_PWM.astype(int).tolist()}", flush=True)
        print(f"session_pwm={initial_pwm.astype(int).tolist()}", flush=True)
        print(f"session_q_deg={deg(robot_q)}", flush=True)
        print(f"signs={SIGNS.astype(int).tolist()}", flush=True)
        print("Move the master slowly; Ctrl-C stops streaming.", flush=True)

        period = 1.0 / args.rate
        max_step = math.radians(args.max_step_deg)
        filtered_q = robot_q.copy()
        session_pwm = initial_pwm.copy()
        session_q = robot_q.copy()
        last_fresh = time.monotonic()
        next_status = time.monotonic()

        while running:
            started = time.monotonic()
            pwm = reader.read_all()
            if not running:
                break
            now = time.monotonic()
            if now - last_fresh > args.stale_timeout:
                raise RuntimeError("master input stale; stopping teleoperation")
            if pwm is None:
                # Do not resend a cached target as if it were new input.
                time.sleep(0.002)
                continue
            last_fresh = now

            delta_pwm = pwm - session_pwm
            delta_pwm[np.abs(delta_pwm) <= 2.0] = 0.0
            effective_pwm = session_pwm + delta_pwm
            desired_q = master_pwm_to_i3_joints(
                effective_pwm,
                max_offset_deg=args.max_offset_deg,
                reference_pwm=session_pwm,
                reference_q=session_q,
            )
            if args.max_step_deg > 0.0:
                filtered_q += np.clip(desired_q - filtered_q, -max_step, max_step)
            else:
                # No extra script-side lag; ServoJ/Ruckig in the controller
                # still enforces the cabinet's configured motion limits.
                filtered_q = desired_q.copy()

            # Master feedback is read at tens of Hz.  ServoJ lets the cabinet's
            # Ruckig planner shape each target at the 1 kHz control period;
            # JointPassthrough would interpret a 1-degree frame jump as an
            # instantaneous 1 ms jump and trigger the velocity soft limit.
            result = robot.ServoJ(rcore_sdk.JointPositions(filtered_q.tolist()))
            if not result_ok(result):
                raise RuntimeError(f"ServoJ rejected: {result}")

            now = time.monotonic()
            if now >= next_status:
                print(
                    f"pwm={pwm.astype(int).tolist()} "
                    f"desired_deg={deg(desired_q)} cmd_deg={deg(filtered_q)}",
                    flush=True,
                )
                next_status = now + 0.5
            remaining = period - (time.monotonic() - started)
            if remaining > 0.0:
                time.sleep(remaining)

        return 0
    finally:
        try:
            if connected:
                stopped = robot.StopMotion()
                if not result_ok(stopped):
                    print(f"StopMotion rejected: {stopped}", flush=True)
        except Exception as error:
            print(f"StopMotion failed: {error}", flush=True)
        try:
            if robot is not None:
                robot.Shutdown()
        except Exception:
            pass
        reader.close()
        print("Teleoperation stopped; master torque was not restored automatically.", flush=True)


if __name__ == "__main__":
    raise SystemExit(main())
