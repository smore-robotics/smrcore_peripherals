# Troubleshooting

## Pico teleoperation is missing

Default packages contain readers and the original bridge. Build the optional executable with a compatible dual-arm SDK as described in [robot integration](robot-integration.md).

## Python import fails

Check that installation and execution use the same interpreter:

```bash
which python
python -m pip show rcore-peripherals-py
```

## Cannot open a device

Check the actual input or serial node and user permissions. See [platform setup](https://github.com/smore-robotics/smrcore_peripherals/blob/main/docs/platform_setup.en.md).

## Pico has no pose

Enter VR, verify tracking and the browser secure context. See [Pico connection](readers.md).

## Robot connection fails

Check cabinet services, target IP, robot instance and DDS settings. Receiving peripheral samples does not establish a robot connection.

## New files are missing in Releases

Ordinary pushes create Actions artifacts; version tags publish releases. Check the correct fork and commit, as explained in [downloads](downloads.md).
