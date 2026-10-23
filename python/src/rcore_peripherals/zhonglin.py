"""Shared constants and helpers for Zhonglin master -> i3 teleoperation."""

from __future__ import annotations

import math
import re
import time
from typing import Sequence

import numpy as np


SERVO_IDS = tuple(range(6))
HOME_PWM = np.array([1566, 1533, 1935, 1345, 1036, 1469], dtype=float)
I3_HOME_DEG = np.array([0.0, -90.0, 0.0, -90.0, 0.0, 0.0])
# Direction mapping for the current master/slave mounting.  All six axes are
# inverted relative to the previous calibration because the slave appeared
# mirrored during forward teleoperation.
SIGNS = np.array([1.0, -1.0, 1.0, -1.0, 1.0, -1.0])
# Per-joint PWM->angle scale (J1..J6). J5 needs 2x on this hardware pairing.
JOINT_PWM_SCALE = np.array([1.0, 1.0, 1.0, 1.0, 2.0, 1.0])
RAD_PER_PWM = math.radians(270.0 / 2000.0)

# Controller limits observed from the SMR-i3 model. Keep a small margin.
JOINT_MIN_DEG = np.array([-359.0, -184.0, -151.0, -269.0, -129.0, -359.0])
JOINT_MAX_DEG = np.array([359.0, 4.0, 151.0, 89.0, 129.0, 359.0])

ALIGN_TOLERANCE_DEG = 5.0


def result_ok(result: object) -> bool:
    if result is None:
        return False
    if isinstance(result, dict):
        return bool(result.get("success", False))
    if isinstance(result, bool):
        return result
    return bool(getattr(result, "success", False))


def deg(values: Sequence[float]) -> list[float]:
    return [round(math.degrees(float(value)), 2) for value in values]


class ZhonglinReader:
    def __init__(self, port: str, baudrate: int) -> None:
        import serial
        self.serial = serial.Serial(port, baudrate, timeout=0.006, write_timeout=0.1)


    def close(self) -> None:
        self.serial.close()

    def command(self, command: str, response_timeout: float = 0.08) -> bytes:
        self.serial.reset_input_buffer()
        self.serial.write(command.encode("ascii"))
        self.serial.flush()
        deadline = time.monotonic() + response_timeout
        response = bytearray()
        while time.monotonic() < deadline:
            chunk = self.serial.read(32)
            if chunk:
                response.extend(chunk)
                if b"!" in response:
                    break
        return bytes(response)

    def read_pwm(self, servo_id: int, retries: int = 3) -> int | None:
        for attempt in range(retries):
            response = self.command(f"#{servo_id:03d}PRAD!")
            match = re.search(rb"#%03dP(\d{4})!" % servo_id, response)
            if match is None:
                if attempt + 1 < retries:
                    time.sleep(0.006)
                continue
            value = int(match.group(1))
            if 500 <= value <= 2500:
                return value
            if attempt + 1 < retries:
                time.sleep(0.006)
        return None

    def read_all(self) -> np.ndarray | None:
        """Return only a fresh complete six-axis sweep; never reuse stale axes."""
        values = []
        for servo_id in SERVO_IDS:
            value = self.read_pwm(servo_id, retries=1)
            if value is None:
                return None
            values.append(float(value))
        return np.asarray(values, dtype=float)

    def release_torque(self) -> None:
        replies = []
        for servo_id in SERVO_IDS:
            reply = self.command(f"#{servo_id:03d}PULM!", response_timeout=0.15)
            if not reply:
                reply = self.command(f"#{servo_id:03d}PULK!", response_timeout=0.15)
            replies.append(reply.decode("ascii", errors="replace") or "<no reply>")
            time.sleep(0.15)
        print("Master torque release:", replies, flush=True)
        time.sleep(0.2)



def master_pwm_to_i3_joints(
    pwm: Sequence[float],
    *,
    max_offset_deg: float = 0.0,
    reference_pwm: Sequence[float] | None = None,
    reference_q: Sequence[float] | None = None,
) -> np.ndarray:
    """Map master PWM to i3 joint targets [rad].

    Default (absolute): reference pose is I3_HOME at HOME_PWM.
    Relative mode: pass reference_pwm/reference_q captured at teleop start so
    master motion maps 1:1 from the aligned posture.
    """
    pwm_arr = np.asarray(pwm, dtype=float)
    if pwm_arr.shape != (6,) or not np.all(np.isfinite(pwm_arr)) or np.any((pwm_arr < 500) | (pwm_arr > 2500)):
        raise ValueError("expected six finite PWM values within 500..2500")
    if (reference_pwm is None) != (reference_q is None):
        raise ValueError("reference_pwm and reference_q must be supplied together")
    for ref in (reference_pwm, reference_q):
        if ref is not None and (np.asarray(ref).shape != (6,) or not np.all(np.isfinite(ref))):
            raise ValueError("invalid six-axis reference")
    if not math.isfinite(max_offset_deg) or not 0 <= max_offset_deg <= 180:
        raise ValueError("invalid offset cap")
    joint_min = np.radians(JOINT_MIN_DEG)
    joint_max = np.radians(JOINT_MAX_DEG)
    if reference_pwm is not None and reference_q is not None:
        ref_pwm = np.asarray(reference_pwm, dtype=float)
        ref_q = np.asarray(reference_q, dtype=float)
        limit_room = np.minimum(ref_q - joint_min, joint_max - ref_q)
    else:
        ref_pwm = HOME_PWM
        ref_q = np.radians(I3_HOME_DEG)
        limit_room = np.minimum(ref_q - joint_min, joint_max - ref_q)

    if np.any(ref_q < joint_min) or np.any(ref_q > joint_max):
        raise ValueError("reference pose is outside the i3 example joint limits")

    delta = SIGNS * (pwm_arr - ref_pwm) * RAD_PER_PWM * JOINT_PWM_SCALE
    if max_offset_deg > 0.0:
        cap = np.minimum(np.radians(max_offset_deg), limit_room)
        delta = np.clip(delta, -cap, cap)
    return np.clip(ref_q + delta, joint_min, joint_max)


def max_joint_error_deg(actual_rad: Sequence[float], target_rad: Sequence[float]) -> float:
    actual = np.asarray(actual_rad, dtype=float)
    target = np.asarray(target_rad, dtype=float)
    return float(np.max(np.abs(np.degrees(actual - target))))


