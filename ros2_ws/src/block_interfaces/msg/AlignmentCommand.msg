# error_x: target centre vs image centre. error_y: target BOTTOM edge vs bottom_target line.
# Errors are normalized by half image size. Rates are rad/s; receiver must enforce TTL.
std_msgs/Header header
uint64 track_id
bool valid
bool predicted
float64 error_x
float64 error_y
float64 yaw_rate
float64 pitch_rate
# Chassis forward speed in m/s (>= 0). camera_servo on: only until the camera has reached the grasp pitch.
# camera_servo off (fixed camera): only while the block's bottom edge is above the target band. pitch_rate is then 0.
float64 forward_speed
bool aligned
# One-shot request per lock. External executor acknowledges by resetting the lock.
bool grasp_request
float64 valid_for_sec
string reason
