# smrcore_peripherals

**读取 SpaceMouse、六维力传感器和 Pico，并通过 Smartmore Robot SDK 集成机器人应用。**

[English](README.en.md) · **简体中文**

---

这是 Smartmore 外设 SDK 的公开源码仓库，提供 C++ 外设库、Python wheel、读数示例及可选机器人 SDK bridge。外设读取不连接机器人；控制由独立 bridge 组合 [smrcore_sdk](https://github.com/smore-robotics/smrcore_sdk)。

## 文档

- [C++ 应用](app/README.md)
- [Python 应用](python/app/README.md)
- [设备权限与平台配置](docs/platform_setup.md)
- [Pico 遥操链路与适用范围](python/TELEOP.md)
- [制品、配套 SDK 与发布流程](docs/releases.md)
- [Release 下载页](https://github.com/smore-robotics/smrcore_peripherals/releases)：正式版本的中英文 PDF / HTML 说明书随制品提供。

## 快速开始

### C++ 外设读取

Linux x86_64，建议 Ubuntu 22.04。需要 GCC/CMake、Conan 2 和 Python 3.10+。

```bash
git clone https://github.com/smore-robotics/smrcore_peripherals.git
cd smrcore_peripherals
python3 -m pip install 'conan>=2,<3'
./scripts/build.sh --with-sdk OFF --tests ON
./scripts/run_tests.sh -t Release
./build_Release/bin/app_peripherals_probe
./build_Release/bin/app_peripherals_read_pico --port 8765
```

### Python

基础 wheel 只读取外设，不打包机器人 SDK 或遥操核心。

```bash
./scripts/build_py.sh --no-install
python3 -m pip install --force-reinstall python/dist/*.whl
python3 python/app/app_peripherals_read_pico.py --port 8765
```

也可从 Releases 下载与解释器匹配的 Linux x86_64 wheel：CPython 3.10 / 3.11 / 3.12。未打标签的开发 wheel 版本为 0.0.0，请通过对应 Actions 提交识别构建。

### 机器人 SDK bridge

```bash
./scripts/download.sh
./scripts/build.sh --with-sdk ON --with-pico OFF
```

默认 SDK 版本由 [.sdk-version](.sdk-version) 固定，当前为 **0.2.0**。原 SpaceMouse/F/T bridge 使用该版本；运行参数见应用文档。

Pico 双臂 bridge 另行通过 `--with-pico ON --sdk-root /path/to/compatible-sdk` 构建。官方 v0.2.0 原包漏装双臂头文件，必须使用含 `sdk/dual_arm_robot.hpp` 和匹配运行库的修复包，不能直接使用原包。该示例**内置 B3 支架变换，仅适用于对应 B3 双臂安装构型**；实机行为尚待验收，默认发布制品不包含它。

## Release 制品

| 制品 | 内容 |
|---|---|
| `smrcore_peripherals-cpp-linux-x86_64-v<version>.tar.gz` | 外设 C++ 库、头文件、CMake 配置、读取程序和 Python 示例 |
| `smrcore_peripherals-sdk-bridge-linux-x86_64-v<version>.tar.gz` | 原 SpaceMouse/F/T bridge、标定工具及配套 SDK 运行库 |
| `rcore_peripherals_py-<version>-cp3xx-cp3xx-linux_x86_64.whl` | CPython 3.10–3.12 外设读取 wheel |
| `smrcore_peripherals-manual-zh-v<version>.pdf/html` | 中文说明书 |
| `smrcore_peripherals-manual-en-v<version>.pdf/html` | 英文说明书 |
| `SHA256SUMS` | 正式发布制品校验和 |

每次 push/PR 构建后在 [Actions](https://github.com/smore-robotics/smrcore_peripherals/actions) 提供制品；推送 `vX.Y.Z` 标签且检查成功后自动建立 Release。首次 CI 成功前，上表表示配置的发布范围，不表示已有可下载版本。当前不承诺 Windows、ARM64 或模拟器制品。

解压 C++ 包后通过 `./run.sh app_peripherals_read_pico --port 8765` 运行；wrapper 设置包内库路径。硬件读取仍需相应设备权限。

## 安全

机器人 bridge 会影响真实机器人。执行前确认机器人、工具、负载、工作区和示例适配，急停可触达；编译或模拟测试通过不代表实机验收通过。

## 许可证

仓库采用 [Apache License 2.0](LICENSE)。SDK bridge 包随 SDK 运行库保留其第三方许可证和归属说明。
