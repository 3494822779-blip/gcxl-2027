# error_x: target centre vs image centre. error_y: target BOTTOM edge vs bottom_target line.
# Errors are normalized by half image size. Rates are rad/s; receiver must enforce TTL.
# For valid commands header.stamp is the source observation time, not the controller tick.
std_msgs/Header header
uint64 track_id
bool valid
bool predicted
float64 error_x
float64 error_y
float64 yaw_rate
float64 pitch_rate
# Chassis forward speed in m/s (negative = reverse). camera_servo on: only until the camera has reached the grasp pitch.
# camera_servo off: forward above the target band, reverse below it when centred; pitch_rate=0.
float64 forward_speed
bool aligned
# Level-triggered readiness; executor owns sweep/opening/grasp admission.
bool grasp_request
float64 valid_for_sec
string reason

# Nonzero identifies a target-grasp task. clearance is deprecated and always false.
uint64 first_grasp_id
bool clearance
int32 class_id

# Obstacle metadata relative to the locked formal target. -1=left, +1=right, 0=none.
uint64 obstacle_id
int8 sweep_side
float64 obstacle_overlap
bool obstacle_near
bool open_allowed
