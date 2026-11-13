# Executor search state. Only applies to this active target-grasp task.
std_msgs/Header header
uint64 task_id
bool active
# Wheels commanded stopped and camera feedback at the held pitch during a dwell.
# This is a command/angle condition, not physical chassis stop confirmation.
bool stationary
