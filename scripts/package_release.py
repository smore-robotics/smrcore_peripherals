#!/usr/bin/env python3
"""Package an installed Linux peripheral build without rebuilding or robot access."""
import argparse
import hashlib
import json
import platform
import re
import shutil
import subprocess
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=ROOT / "dist")
    parser.add_argument("--version", required=True)
    parser.add_argument("--sdk-root", type=Path)
    args = parser.parse_args()
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:-dev\.[0-9a-f]{7,40})?", args.version):
        parser.error("version must be x.y.z or x.y.z-dev.<commit>")
    if platform.system() != "Linux" or platform.machine() != "x86_64":
        parser.error("release packaging currently supports Linux x86_64 only")
    install = args.install.resolve()
    if not (install / "lib/libsmrcore_peripherals.so").is_file():
        parser.error("missing installed peripheral shared library")
    sdk_version = (ROOT / ".sdk-version").read_text().strip()
    flavor = "sdk-bridge" if args.sdk_root else "cpp"
    name = f"smrcore_peripherals-{flavor}-linux-x86_64-v{args.version}"
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as temp:
        stage = Path(temp) / name
        shutil.copytree(install, stage, symlinks=True)
        if args.sdk_root:
            sdk = args.sdk_root.resolve()
            version_header = sdk / "include/rcore/sdk/version.hpp"
            match = re.search(r'^#define RCORE_SDK_VERSION_STRING "([^"]+)"',
                              version_header.read_text(), re.MULTILINE) if version_header.is_file() else None
            if not match or match.group(1) != sdk_version:
                parser.error("SDK runtime package version does not match .sdk-version")
            if not (sdk / "lib/librcoresdk.so").is_file():
                parser.error("SDK root has no librcoresdk.so")
            if not (stage / "bin/app_peripherals_bridge").is_file():
                parser.error("install tree has no SDK bridge executable")
            shutil.copytree(sdk / "lib", stage / "lib", dirs_exist_ok=True, symlinks=True)
            shutil.copytree(sdk / "include", stage / "include", dirs_exist_ok=True)
            if (sdk / "share").exists():
                shutil.copytree(sdk / "share", stage / "share", dirs_exist_ok=True)
            # Preserve the SDK's top-level attribution as well.
            for source in sdk.iterdir():
                if source.is_file() and any(x in source.name.upper() for x in ("LICENSE", "NOTICE", "COPYING")):
                    dest = stage / "share/sdk-attribution"
                    dest.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(source, dest / source.name)
        elif (stage / "bin/app_peripherals_bridge").exists():
            parser.error("base package must be built with --with-sdk OFF")
        shutil.copy2(ROOT / "LICENSE", stage / "LICENSE")
        shutil.copy2(ROOT / "README.md", stage / "README.md")
        shutil.copy2(ROOT / "README.en.md", stage / "README.en.md")
        shutil.copytree(ROOT / "docs", stage / "docs")
        (stage / "app").mkdir(exist_ok=True)
        for doc_name in ("README.md", "README.en.md"):
            shutil.copy2(ROOT / "app" / doc_name, stage / "app" / doc_name)
        (stage / "python").mkdir(exist_ok=True)
        shutil.copy2(ROOT / "python/TELEOP.md", stage / "python/TELEOP.md")
        shutil.copytree(ROOT / "python/app", stage / "python/app",
                        ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
        (stage / "run.sh").write_text(
            '#!/usr/bin/env bash\nset -euo pipefail\n'
            'root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"\n'
            'if [[ $# -eq 0 || "$1" == */* || ! -x "$root/bin/$1" ]]; then\n'
            '  echo "Usage: ./run.sh <binary-name> [arguments]" >&2; exit 2\nfi\n'
            'binary="$1"; shift\n'
            'export LD_LIBRARY_PATH="$root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
            'exec "$root/bin/$binary" "$@"\n')
        (stage / "run.sh").chmod(0o755)
        metadata = {
            "version": args.version,
            "commit": subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip(),
            "dirty": bool(subprocess.check_output(
                ["git", "-C", str(ROOT), "status", "--porcelain"], text=True).strip()),
            "platform": "linux-x86_64",
            "sdk_version": sdk_version if args.sdk_root else None,
            "pico_teleop_included": (stage / "bin/app_peripherals_pico_bridge").exists(),
        }
        (stage / "build-info.json").write_text(json.dumps(metadata, indent=2) + "\n")
        archive = args.output / f"{name}.tar.gz"
        with tarfile.open(archive, "w:gz") as out:
            out.add(stage, arcname=name)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    archive.with_name(archive.name + ".sha256").write_text(f"{digest}  {archive.name}\n")
    print(archive)


if __name__ == "__main__":
    main()
