# Errors are normalized by half image size. Rates are rad/s; receiver must enforce TTL.
std_msgs/Header header
uint64 track_id
bool valid
bool predicted
float64 error_x
float64 error_y
float64 yaw_rate
float64 pitch_rate
bool aligned
# One-shot request per lock. External executor acknowledges by resetting the lock.
bool grasp_request
float64 valid_for_sec
string reason
