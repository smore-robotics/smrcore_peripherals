# Pico Reading and Dual-arm Teleoperation

## Architecture

C++ `app_peripherals_read_pico` and Python `app_peripherals_read_pico.py` read headset/controller poses and buttons without a robot SDK. The optional C++ `app_peripherals_pico_bridge` maps poses and controls B3 arms through the dual-arm SDK.

SpaceMouse uploads samples for the cabinet to consume; Pico maps targets in the application and calls SDK motion APIs. Both separate reading from control, but use different control paths.

## Read-only check

```bash
./scripts/build.sh --with-sdk OFF --tests ON
./build_Release/bin/app_peripherals_read_pico --port 8765
```

Or, after installing the reader wheel:

```bash
python python/app/app_peripherals_read_pico.py --port 8765
```

Run only one server on a port. With USB debugging authorized, use another terminal:

```bash
adb devices
adb reverse tcp:8765 tcp:8765
```

Open `http://localhost:8765/` in the headset and enter VR. Output includes sequence, reference space, headset/left/right XYZ in meters, grip and trigger values. These are WebXR reference-space positions, not robot Base/TCP coordinates. Untracked positions are N/A; `GetSample()` is empty when no new sample exists.

For ordinary Wi-Fi, localhost means the headset. Use an accessible trusted HTTPS endpoint with WebSocket forwarding; replacing localhost with an HTTP PC address does not establish a WebXR secure context. The reproducible commands above use adb and do not alter robot network routes.

## Build and start the optional bridge

```bash
./scripts/build.sh --with-sdk ON --with-pico ON --sdk-root /path/to/compatible-sdk
./build_Release/bin/app_peripherals_pico_bridge --help
```

The SDK must contain `sdk/dual_arm_robot.hpp` and matching libraries. The original official v0.2.0 package omitted this header. Default CI/packages exclude Pico teleoperation. The example embeds B3 mounting-bracket transforms and is not a generic dual-arm configuration. Connection uses `DualArmRobot::Initialize(robot_ip)` without CLI domain/interface overrides.

The following enters real robot control. Verify read-only input, mounting, tool and workspace first and keep E-stop accessible. Stop the reader before starting the bridge:

```bash
./build_Release/bin/app_peripherals_pico_bridge --enable-motion --robot-ip <robot-ip> --port 8765
```

| Input | Behavior |
| --- | --- |
| Hold Grip | Clutch and control the corresponding arm from pose increments |
| Release Grip | Cancel old targets and perform a planned ServoJ stop |
| Thumbstick Y while gripping | Control J6 with ServoJ; neutral re-clutches ServoP from measured TCP |
| Trigger rising edge | Toggle corresponding DH gripper; press 0.75, release 0.25 hysteresis |
| Ctrl+C | Request stop and cleanup |

**Startup initializes both DH grippers, sets force/speed and requests opening.** Write success is not physical position or grasp confirmation; no readback is implemented.

## CLI options

| Option | Default / range | Meaning |
| --- | --- | --- |
| `--enable-motion` | Required explicitly | Real-control confirmation |
| `--robot-ip` | Cabinet address | Dual-arm SDK connection |
| `--port` | 8765 / 1–65535 | WebXR port |
| `--translation-only` | Off | Disable rotation mapping |
| `--room-fixed` | Off | Room-fixed instead of head-yaw-clutched direction |
| `--operator-frame` | same-side; also face-to-face | Operator direction |
| `--translation-scale` | 0.8 / 0.1–3.0 | Translation mapping |
| `--rotation-scale` | 1.0 / 0.1–3.0 | Rotation mapping |
| `--motion-scale` | Unset / 0.1–3.0 | Set both scales |
| `--j6-speed` | 0.8 rad/s / 0.1–3.0 | Requested J6 speed, subject to controller limits |
| `--gripper-force` | 80 / 20–100 | DH force parameter, not newtons |
| `--gripper-speed` | 100 / 1–100 | DH speed parameter |

Arguments are processed in order; later options override earlier scale settings.

## Holds and recovery

Approaching singularities uses quadratic slowdown between condition numbers 30 and 60; leaving uses doubled thresholds. A transient stale Jacobian freezes/brakes the affected arm. Pico input older than 250 ms freezes/brakes both arms and cancels unsent gripper commands; recovered input re-clutches from measured TCP. Stale robot state, controller faults and guard infrastructure failures terminate the session.

Pending/rejected candidate checks hold the last approved target. J6 bypasses Cartesian singularity scaling but retains joint constraints. These describe source behavior, not completed hardware acceptance. No independent Python dual-arm control example is provided.

## Start Pico dual-arm teleoperation from Python

The Python entry point reuses the C++ SDK bridge, including the B3 mounting transform, Grip clutch, J6 thumbstick, Trigger grippers, singularity slowdown and stale-input stop. Neither the peripheral wheel nor the robot Python SDK is required.

Build with the compatible dual-arm C++ SDK:

```bash
./scripts/build.sh --with-sdk ON --with-pico ON --sdk-root /path/to/compatible-sdk
python3 python/app/app_peripherals_pico_bridge.py --help
```

After checking the workspace and emergency stop:

```bash
python3 python/app/app_peripherals_pico_bridge.py \
  --robot-ip 192.168.3.232 --port 8765 --enable-motion \
  --gripper-force 80 --gripper-speed 100
```

Use `--bridge-executable /path/to/app_peripherals_pico_bridge` for an installed binary. Otherwise the launcher checks repository `build_Release/bin`, `build_Release/install/bin`, then PATH. Other arguments pass unchanged to the C++ parser; motion opt-in is never added automatically.
The Python process is replaced by the bridge, preserving Ctrl+C handling and exit status without leaving an extra background motion process. Do not run the Pico reader on the same port simultaneously; use the Pico guide for headset connection steps.
