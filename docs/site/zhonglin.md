# Master–follower arms

Master input → joint mapping → robot SDK `ServoJ` → follower arm.

## Read the master

After installing the peripherals wheel:

```bash
python3 -m pip install numpy pyserial
python3 python/app/app_peripherals_read_zhonglin.py --port /dev/ttyUSB0
```

The reader prints PWM and mapped joint angles in degrees without connecting to a robot or changing master torque. The wheel's `zhonglin` extra also installs these dependencies.

## Control one i3

Install the robot Python SDK (`rcore_sdk`) with `Robot.Initialize/GetState/Enable/MoveJ/ServoJ/StopMotion/Shutdown`. Verify the robot, workspace and emergency stop before running:

```bash
python3 python/app/app_peripherals_zhonglin_i3_bridge.py \
  --port /dev/ttyUSB0 --robot-ip 192.168.3.232 --execute
```

This enables the i3. Default operation uses relative startup anchors and a nominal 20 Hz rate (5–50 Hz configurable). Master torque is released unless `--keep-master-torque` is set; exit does not restore it.
`--align-start` first moves the robot to the master's absolute mapped pose using MoveJ at a default velocity scale of 0.1.

Incomplete samples do not resend cached targets. A complete-input timeout (`--stale-timeout`, default 0.5 s), rejected ServoJ, exception or Ctrl+C requests StopMotion. Stop requests can fail if communication is lost; cabinet communication and motion protection must remain active.
`--max-offset-deg` caps motion around the startup pose; `--max-step-deg` caps each transmitted joint step. Both default to 0 (disabled; example joint bounds still apply).

The original HOME PWM, directions, J5 scale of 2 and i3 bounds reside in `python/src/rcore_peripherals/zhonglin.py`. They describe this particular master/i3 pairing and require calibration review for another installation. This integration has software-only validation; real-robot acceptance has not been performed.
