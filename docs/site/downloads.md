# Release Notes and Downloads

- [Releases](https://github.com/smore-robotics/smrcore_peripherals/releases): published after version-tag checks pass.
- [Actions](https://github.com/smore-robotics/smrcore_peripherals/actions): development artifacts at the bottom of each run; GitHub login is required, retention is 14 days.

Fork builds appear in the fork's Actions, not the upstream Releases page. A release containing only Source code attachments has no uploaded binaries.

| Package | Contents |
| --- | --- |
| cpp-linux-x86_64 | C++ readers, headers and library |
| sdk-bridge-linux-x86_64 | Original SpaceMouse/F/T bridge, calibration tool and SDK libraries; no Pico teleoperation |
| rcore_peripherals_py wheel | Reader library for CPython 3.10 / 3.11 / 3.12 |
| manual-zh / manual-en | PDF and HTML manuals |
| SHA256SUMS | Tagged release checksums |

After extracting the C++ package:

```bash
./run.sh app_peripherals_read_pico --port 8765
```

Install exactly one wheel matching your interpreter:

```bash
python -m pip install ./rcore_peripherals_py-*.whl
```

Example Python scripts are in the source repository's `python/app/`. This site follows the deployment branch; consult the bundled manual for an older release. Windows and ARM64 binaries are not currently promised.
