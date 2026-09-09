# piper_isaacsim

Lets MoveIt (`piper_with_gripper_moveit`) drive the Piper arm running in
Isaac Sim (the `go2_in_isaacsim` extension), the same way `piper_gazebo`
does for Gazebo -- via
[`topic_based_ros2_control`](https://github.com/PickNikRobotics/topic_based_ros2_control)
bridged to Isaac Sim's `piper/joint_states` / `piper/joint_command` topics
instead of `gazebo_ros2_control`. Only the hardware-interface plugin
differs from `piper_gazebo`'s own `ros2_controllers.yaml` / controller
layer (`arm_controller`, `gripper_controller`, `gripper8_controller`, all
`FollowJointTrajectory`) -- MoveIt's `moveit_controllers.yaml` needs no
changes.

## Precondition

Isaac Sim must already be running before/while this launch runs --
`go2_in_isaacsim`, "Go2 with Mid-360 + Piper" robot preset, **ROS2 Bridge**
enabled in Preferences, and **Play** pressed. There's no "spawn into Isaac
Sim" step here (unlike Gazebo's `spawn_entity.py`) -- the articulation
already exists in the running Isaac Sim stage; this package just attaches a
`ros2_control` hardware interface to its existing joint topics.

## Setup

```bash
sudo apt install ros-humble-topic-based-ros2-control  # if not already present
colcon build --packages-select piper_isaacsim
```

## Run

```bash
source install/setup.bash
ros2 launch piper_isaacsim piper_isaacsim_bringup.launch.py
```

Starts `robot_state_publisher` + `ros2_control_node` (topic-based hardware
interface) + controller spawners (`joint_state_broadcaster`,
`arm_controller`, `gripper_controller`, `gripper8_controller`) +
`piper_gazebo`'s own `joint8_ctrl.py` gripper-mirror node (joint8 has no
true mimic joint in the URDF) + MoveIt's `move_group` + RViz.

## Verify

```bash
ros2 control list_controllers   # all should show "active"
```

Then Plan & Execute a goal in RViz's Motion Planning panel and confirm the
arm actually moves in Isaac Sim.

## Known limitations

- `gripper8_controller`'s mirroring is driven entirely by
  `piper_gazebo/joint8_ctrl.py`; MoveIt itself only commands `joint7`
  (`gripper_controller`).
- Assumes Isaac Sim's Piper articulation reports joint names `joint1`..
  `joint8` verbatim (verified headlessly against the bundled
  `go2_with_mid360_and_piper.usd`) -- a different Piper USD/description
  variant with different joint names would need this package's xacro/config
  updated to match.
