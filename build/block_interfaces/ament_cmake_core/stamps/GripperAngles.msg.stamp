# Perception proposal only, not a motor command. Angles are in radians.
# Consumers MUST require angles_ready and reject expired acquisition timestamps.
std_msgs/Header header
uint64 track_id
uint32 image_width
uint32 image_height
bool bounds_valid
float64 left_edge_px
float64 right_edge_px
bool angles_ready
# NaN until the calculation method is implemented; zero is NOT a fallback angle.
float64 left_angle_rad
float64 right_angle_rad
float64 valid_for_sec
string reason
