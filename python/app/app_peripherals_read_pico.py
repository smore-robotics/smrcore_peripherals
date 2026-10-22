#!/usr/bin/env python3
"""Read Pico WebXR positions and buttons without connecting to a robot."""

import argparse
import time


def format_pose(label, pose):
    position = (
        "(" + ",".join(f"{v:.4f}" for v in pose.position) + ")"
        if pose.tracked else "N/A"
    )
    return f"{label}={'tracked' if pose.tracked else 'missing'} xyz[m]={position}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    if not 1 <= args.port <= 65535:
        parser.error("--port must be between 1 and 65535")

    from rcore_peripherals import Pico, PicoOptions

    pico = Pico()
    try:
        if not pico.Initialize(PicoOptions(port=args.port)) or not pico.Start():
            print(f"Pico startup failed: {pico.GetLastError()}", flush=True)
            return 1
        print(f"adb reverse tcp:{args.port} tcp:{args.port}", flush=True)
        print(f"Open http://localhost:{args.port}/ in Pico and enter VR. Ctrl+C exits.", flush=True)
        while True:
            sample = pico.GetSample()
            if sample is not None:
                print(
                    f"seq={sample.sequence} ref={sample.reference_space} "
                    f"{format_pose('headset', sample.headset)} "
                    f"{format_pose('L', sample.left)} "
                    f"grip={sample.left.grip:.2f} trigger={sample.left.trigger:.2f} "
                    f"{format_pose('R', sample.right)} "
                    f"grip={sample.right.grip:.2f} trigger={sample.right.trigger:.2f}",
                    flush=True,
                )
            time.sleep(0.002)
    except KeyboardInterrupt:
        return 0
    finally:
        try:
            pico.Stop()
        finally:
            pico.Shutdown()


if __name__ == "__main__":
    raise SystemExit(main())
