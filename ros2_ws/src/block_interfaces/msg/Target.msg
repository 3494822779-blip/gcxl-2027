std_msgs/Header header
uint32 image_width
uint32 image_height
bool locked
bool observed
bool valid
uint64 track_id
int32 class_id
float64 raw_x
float64 raw_y
float64 x
float64 y
builtin_interfaces/Time last_observed

# Nonzero identifies a target-grasp task. clearance is deprecated and always false.
uint64 first_grasp_id
bool clearance

# Obstacle metadata relative to the locked formal target. -1=left, +1=right, 0=none.
uint64 obstacle_id
int8 sweep_side
float64 obstacle_overlap
bool obstacle_near
