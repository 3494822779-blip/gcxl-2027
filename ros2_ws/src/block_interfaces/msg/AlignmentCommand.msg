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
# Chassis forward speed in m/s (>= 0); non-zero only while the camera has not yet reached the grasp pitch band.
float64 forward_speed
bool aligned
# One-shot request per lock. External executor acknowledges by resetting the lock.
bool grasp_request
float64 valid_for_sec
string reason
