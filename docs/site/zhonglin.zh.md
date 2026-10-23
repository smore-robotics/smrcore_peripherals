# 主从臂

主臂输入 → 六轴关节映射 → 机器人 SDK `ServoJ` → 从臂跟随。

## 安装与读取

安装外设 wheel 后，补充串口和数值计算依赖：

```bash
python3 -m pip install numpy pyserial
python3 python/app/app_peripherals_read_zhonglin.py --port /dev/ttyUSB0
```

输出六轴 PWM 和映射后的 i3 关节角（度）。读取程序不连接机器人、不改变主臂力矩。
也可安装 `rcore-peripherals-py` 的 `zhonglin` extra 获取上述依赖。

## 单臂遥操

另外安装机器人 Python SDK `rcore_sdk`，使用提供 `Robot.Initialize/GetState/Enable/MoveJ/ServoJ/StopMotion/Shutdown` 的版本。
在确认目标机械臂、工作空间和急停可用后执行：

```bash
python3 python/app/app_peripherals_zhonglin_i3_bridge.py \
  --port /dev/ttyUSB0 --robot-ip 192.168.3.232 --execute
```

程序会使能 i3；默认以启动时的主臂位置和 i3 实际位置为相对零点，20 Hz 发送目标。
默认释放主臂舵机力矩，`--keep-master-torque` 可保留。退出不会自动恢复主臂力矩。
`--align-start` 会先执行 MoveJ，将 i3 移到主臂的绝对映射姿态，默认速度比例 0.1。

| 参数 | 默认值 | 含义 |
|---|---|---|
| `--rate` | 20 | 发送频率，5–50 Hz，实际受串口读取耗时限制 |
| `--stale-timeout` | 0.5 | 完整六轴输入超时秒数，超时请求 StopMotion |
| `--max-offset-deg` | 0 | 相对起始位姿的偏移上限；0 使用示例关节限位 |
| `--max-step-deg` | 0 | 每次发送的关节变化上限；0 不增加脚本滤波 |

串口不完整时不发送缓存目标；ServoJ 拒绝或异常时退出并请求 StopMotion。
Ctrl+C 同样请求停止。通信中断时停机请求可能失败，柜端仍需保持自身的通信和运动保护。

## 映射与适用范围

标定保存在 `python/src/rcore_peripherals/zhonglin.py`：原有 HOME PWM、方向和 i3 关节范围保留，J5 比例为 2。
这些值对应原来的中凌主臂与单臂 i3 配对；更换安装或机型需重新核对标定。
本次接入验证覆盖无硬件单测和打包；尚未执行实机验收。
