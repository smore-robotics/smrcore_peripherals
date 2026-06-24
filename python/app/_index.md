> DIR: python/app/

# Overview

对应 C++ `app/` 读数工具与 SDK bridge 的 Python 脚本，依赖已安装的 `rcore-peripherals-py` wheel（import 名 `rcore_peripherals`）；bridge 另需 `rcore_sdk`（`rcore-sdk-py` wheel）。构建与运行说明见 `README.md` / `README.en.md`。

# Contents

- app_peripherals_read_ft_sensor.py: 镜像 `app/app_peripherals_read_ft_sensor.cpp`
  - argparse: `--serial-port` `--sensor-type` `--baud-rate` `--sample-rate` `--read-buffer-size` `--stale-timeout-ms`
- app_peripherals_read_spacemouse.py: 镜像 `app/app_peripherals_read_spacemouse.cpp`（默认 `sample_rate_hz=125`）
- app_peripherals_bridge.py: 镜像 `app/app_peripherals_bridge.cpp`
  - 单一 `Robot` 会话 + `Peripheral().Update*Sample()`；默认两路都启用于 FDCC
  - CLI: `--robot` `--spacemouse` `--ft-sensor` `--spacemouse-device` `--spacemouse-sample-rate` `--ft-serial-port` `--ft-sensor-type` `--ft-baud-rate` `--ft-sample-rate` `--ft-stale-timeout-ms`

运行（需先 `./scripts/build_py.sh`，并安装 `rcore-sdk-py` wheel）:

```bash
python3 python/app/app_peripherals_read_ft_sensor.py --serial-port /dev/ttyUSB0
python3 python/app/app_peripherals_read_spacemouse.py
python3 python/app/app_peripherals_bridge.py --robot 192.168.1.100
python3 python/app/app_peripherals_bridge.py --spacemouse --spacemouse-device /dev/input/event5
```
