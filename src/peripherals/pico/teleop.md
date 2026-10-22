> SRC: teleop.hpp
> REF: ../../../../teleop/_index.md

SDK-backed single-use PicoTeleop: Run, RequestStop, IsRunning, GetExitCode,
GetStatus. Options default to no motion, scales 0.8/1.0 and J6 speed 0.8;
connection uses the upstream DualArmRobot::Initialize(robot_ip) API;
explicit robot IP and
motion confirmation are required. Run uses embedded B3 mounting-bracket
world-from-arm-base matrices for the corresponding B3 mounting configuration, switches both arms to base and applies one
shared rigid-transform chain to translation and rotation. There are no
independent axis or manual yaw corrections. Gripper force/speed default to
80/100; Trigger toggles both
end-board DH grippers independently. While Grip is held, thumbstick Y controls
the corresponding arm J6 through planned ServoJ; neutral re-clutches ServoP
from the measured TCP. Join Run before destroying the object.
