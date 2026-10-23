# 常见问题

## 编译通过，为什么不能直接用 Pico 遥操？

默认构建的是读取器和SpaceMouse / 六维力 bridge。Pico 双臂示例需显式开启 `--with-pico ON` 并提供兼容双臂 SDK，见[接入说明](robot-integration.md)。

## Python 提示找不到 rcore_peripherals

确认安装和运行使用同一环境：

```bash
which python
python -m pip show rcore-peripherals-py
```

## 无法打开输入设备或串口

先检查实际节点和用户权限，不要把示例 `/dev/input/event5` 当成固定设备号。参考[设备权限配置](https://github.com/smore-robotics/smrcore_peripherals/blob/main/docs/platform_setup.md)。

## Pico 页面打开但没有位置

确认进入 VR、手柄处于追踪状态，以及浏览器满足 WebXR 安全上下文要求。USB 转发与 Wi-Fi 地址的区别见[外设读取](readers.md)。

## 机器人 SDK 连接失败

先核对柜端服务、目标 IP、实例和 DDS 配置。外设读取成功只证明输入链路可用，不证明 SDK 已与控制柜建立连接。

## 在 Release 页面看不到新编译包

普通提交的包在 Actions 的 Artifacts；只有版本标签触发正式 Release。Fork 的包也不会出现在上游仓库，见[下载与版本](downloads.md)。
