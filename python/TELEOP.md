# Pico 沿用外设库与机器人 SDK 分离的链路

基础 wheel 只读取 Pico，不连接、使能机器人；与 SpaceMouse 保持相同职责划分。

```bash
./scripts/build.sh --with-sdk OFF --tests OFF
./scripts/build_py.sh
python3 python/app/app_peripherals_read_pico.py --help
```

机器人控制使用独立 bridge 示例，目前已有 C++ 双臂 ServoP 示例：

```bash
./scripts/build.sh --with-sdk ON --with-pico ON --sdk-root /path/to/compatible-sdk
./build_Release/bin/app_peripherals_pico_bridge --help
```

SpaceMouse bridge 上传采样供柜端遥操消费；Pico 当前 bridge 在应用层转换手柄位姿并调用双臂 SDK，
不能把它描述为相同的柜端采样接口。读取层/控制层分离的原则相同。
独立 Python 双臂控制示例尚未提供，基础 wheel 不包含 PicoTeleop 类。
SDK 当前仅支持 Initialize(IP)，需核对 domain 与目标柜的兼容性。

本示例内置 B3 支架变换，适用于对应 B3 双臂安装构型，不是通用双臂配置。其他支架或安装方式需重新配置并验证坐标变换。
