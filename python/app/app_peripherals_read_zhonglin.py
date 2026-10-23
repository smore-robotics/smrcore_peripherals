#!/usr/bin/env python3
"""Read six Zhonglin master PWM values without connecting to a robot."""
import argparse
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--baudrate", type=int, default=115200)
    args = parser.parse_args()
    from rcore_peripherals.zhonglin import ZhonglinReader, master_pwm_to_i3_joints, deg
    reader = ZhonglinReader(args.port, args.baudrate)
    try:
        while True:
            pwm = reader.read_all()
            if pwm is None:
                print("incomplete sample", flush=True)
            else:
                print(f"pwm={pwm.tolist()} mapped_i3_deg={deg(master_pwm_to_i3_joints(pwm))}", flush=True)
            time.sleep(0.05)
    except KeyboardInterrupt:
        return 0
    finally:
        reader.close()


if __name__ == "__main__":
    raise SystemExit(main())
