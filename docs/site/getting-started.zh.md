# 快速开始

本页介绍安装、构建和运行最小外设程序。请选择你的路径：

<div class="grid cards" markdown>

- :material-language-cpp:{ .lg .middle } **C++ SDK**

    ---

    构建外设库和诊断程序。

    [前往 C++ SDK →](#c-sdk)

- :material-language-python:{ .lg .middle } **Python SDK**

    ---

    安装 wheel 并运行读取示例。

    [前往 Python SDK →](#python-sdk)

</div>

## 环境

当前自动构建制品面向 Linux x86_64，建议 Ubuntu 22.04。Python wheel 覆盖 CPython 3.10、3.11、3.12。

源码构建需要 Git、C++17 编译器、CMake、Ninja、Python 和 Conan 2。

```bash
sudo apt-get update
sudo apt-get install -y git build-essential cmake ninja-build python3-venv python3-dev
python3 -m venv .venv-peripherals
source .venv-peripherals/bin/activate
python -m pip install 'conan>=2,<3'
git clone https://github.com/smore-robotics/smrcore_peripherals.git
cd smrcore_peripherals
```

## C++ SDK {#c-sdk}

```bash
./scripts/build.sh --with-sdk OFF --tests ON
./scripts/run_tests.sh -t Release
./build_Release/bin/app_peripherals_probe
```

`--with-sdk OFF` 不连接机器人，也无需安装机器人 SDK。连接设备及权限处理见[外设读取](readers.md)。

## Python SDK {#python-sdk}

继续在仓库根目录、同一个 Python 环境中执行：

```bash
./scripts/build_py.sh --no-install
python -m pip install --force-reinstall python/dist/*.whl
python -c "import rcore_peripherals; print('import OK')"
```

发行包名是 `rcore-peripherals-py`，代码导入名是 `rcore_peripherals`。基础 wheel 不包含机器人 SDK 或 Python 遥操控制核心。

也可以直接安装与解释器匹配的预编译 wheel，见[下载与版本](downloads.md)。

## 下一步

- [读取 SpaceMouse、力传感器和 Pico](readers.md)
- [配置机器人 SDK bridge](robot-integration.md)

详见 [Pico 完整用法](pico.md)和[平台权限配置](platform.md)。
