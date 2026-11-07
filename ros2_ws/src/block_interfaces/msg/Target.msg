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

# Nonzero identifies a first-grasp task; clearance grabs must be discarded.
uint64 first_grasp_id
bool clearance
