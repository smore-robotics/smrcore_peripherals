#!/usr/bin/env python3
"""Start the SDK-backed C++ Pico bridge with the same arguments and behavior.

Build first:
  ./scripts/build.sh --with-sdk ON --with-pico ON --sdk-root /path/to/sdk

Run:
  python3 python/app/app_peripherals_pico_bridge.py --robot-ip ADDRESS --enable-motion

Use --bridge-executable /path/to/app_peripherals_pico_bridge for an installed
binary. All other arguments are passed unchanged to the C++ bridge. --help
shows its options when the binary is available. No Python SDK/wheel is needed.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import sys


BINARY = "app_peripherals_pico_bridge"


def find_bridge(explicit: str | None) -> Path | None:
    if explicit is not None:
        candidate = Path(explicit).expanduser().absolute()
        return candidate if candidate.is_file() and os.access(candidate, os.X_OK) else None
    root = Path(__file__).resolve().parents[2]
    for candidate in (root / "build_Release/bin" / BINARY,
                      root / "build_Release/install/bin" / BINARY):
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate
    found = shutil.which(BINARY)
    return Path(found).absolute() if found else None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, add_help=False,
                                     allow_abbrev=False,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--bridge-executable", metavar="PATH")
    options, forwarded = parser.parse_known_args(argv)
    bridge = find_bridge(options.bridge_executable)
    if "--help" in forwarded or "-h" in forwarded:
        parser.print_help()
        sys.stdout.flush()
        if bridge is None:
            return 0
        forwarded = ["--help"]
    if bridge is None:
        print("Pico C++ bridge not found or not executable. Build with "
              "--with-sdk ON --with-pico ON, or pass --bridge-executable PATH.",
              file=sys.stderr)
        return 2
    try:
        # Replace this process: Ctrl+C/SIGTERM and the exit status belong to the
        # original bridge. Do not create a background motion process or a shell.
        os.execv(str(bridge), [str(bridge), *forwarded])
    except OSError as error:
        print(f"Cannot start {bridge}: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
