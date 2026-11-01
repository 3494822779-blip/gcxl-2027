#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};


#[link(name = "block_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__block_interfaces__msg__Box() -> *const std::ffi::c_void;
}

#[link(name = "block_interfaces__rosidl_generator_c")]
extern "C" {
    fn block_interfaces__msg__Box__init(msg: *mut Box) -> bool;
    fn block_interfaces__msg__Box__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<Box>, size: usize) -> bool;
    fn block_interfaces__msg__Box__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<Box>);
    fn block_interfaces__msg__Box__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<Box>, out_seq: *mut rosidl_runtime_rs::Sequence<Box>) -> bool;
}

// Corresponds to block_interfaces__msg__Box
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]


// This struct is not documented.
#[allow(missing_docs)]

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct Box {

    // This member is not documented.
    #[allow(missing_docs)]
    pub track_id: u64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub class_id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub confidence: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub x: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub y: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub width: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub height: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub observed: bool,

}



impl Default for Box {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !block_interfaces__msg__Box__init(&mut msg as *mut _) {
        panic!("Call to block_interfaces__msg__Box__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for Box {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__Box__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__Box__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__Box__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for Box {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for Box where Self: Sized {
  const TYPE_NAME: &'static str = "block_interfaces/msg/Box";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__block_interfaces__msg__Box() }
  }
}


#[link(name = "block_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__block_interfaces__msg__Frame() -> *const std::ffi::c_void;
}

#[link(name = "block_interfaces__rosidl_generator_c")]
extern "C" {
    fn block_interfaces__msg__Frame__init(msg: *mut Frame) -> bool;
    fn block_interfaces__msg__Frame__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<Frame>, size: usize) -> bool;
    fn block_interfaces__msg__Frame__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<Frame>);
    fn block_interfaces__msg__Frame__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<Frame>, out_seq: *mut rosidl_runtime_rs::Sequence<Frame>) -> bool;
}

// Corresponds to block_interfaces__msg__Frame
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Image and boxes share exactly the same acquisition timestamp and pixel coordinates.

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct Frame {

    // This member is not documented.
    #[allow(missing_docs)]
    pub image: sensor_msgs::msg::rmw::Image,


    // This member is not documented.
    #[allow(missing_docs)]
    pub boxes: rosidl_runtime_rs::Sequence<super::super::msg::rmw::Box>,

}



impl Default for Frame {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !block_interfaces__msg__Frame__init(&mut msg as *mut _) {
        panic!("Call to block_interfaces__msg__Frame__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for Frame {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__Frame__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__Frame__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__Frame__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for Frame {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for Frame where Self: Sized {
  const TYPE_NAME: &'static str = "block_interfaces/msg/Frame";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__block_interfaces__msg__Frame() }
  }
}


#[link(name = "block_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__block_interfaces__msg__Target() -> *const std::ffi::c_void;
}

#[link(name = "block_interfaces__rosidl_generator_c")]
extern "C" {
    fn block_interfaces__msg__Target__init(msg: *mut Target) -> bool;
    fn block_interfaces__msg__Target__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<Target>, size: usize) -> bool;
    fn block_interfaces__msg__Target__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<Target>);
    fn block_interfaces__msg__Target__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<Target>, out_seq: *mut rosidl_runtime_rs::Sequence<Target>) -> bool;
}

// Corresponds to block_interfaces__msg__Target
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]


// This struct is not documented.
#[allow(missing_docs)]

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct Target {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::rmw::Header,


    // This member is not documented.
    #[allow(missing_docs)]
    pub image_width: u32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub image_height: u32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub locked: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub observed: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub valid: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub track_id: u64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub class_id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub raw_x: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub raw_y: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub x: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub y: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub last_observed: builtin_interfaces::msg::rmw::Time,

}



impl Default for Target {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !block_interfaces__msg__Target__init(&mut msg as *mut _) {
        panic!("Call to block_interfaces__msg__Target__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for Target {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__Target__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__Target__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__Target__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for Target {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for Target where Self: Sized {
  const TYPE_NAME: &'static str = "block_interfaces/msg/Target";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__block_interfaces__msg__Target() }
  }
}


#[link(name = "block_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__block_interfaces__msg__AlignmentCommand() -> *const std::ffi::c_void;
}

#[link(name = "block_interfaces__rosidl_generator_c")]
extern "C" {
    fn block_interfaces__msg__AlignmentCommand__init(msg: *mut AlignmentCommand) -> bool;
    fn block_interfaces__msg__AlignmentCommand__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<AlignmentCommand>, size: usize) -> bool;
    fn block_interfaces__msg__AlignmentCommand__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<AlignmentCommand>);
    fn block_interfaces__msg__AlignmentCommand__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<AlignmentCommand>, out_seq: *mut rosidl_runtime_rs::Sequence<AlignmentCommand>) -> bool;
}

// Corresponds to block_interfaces__msg__AlignmentCommand
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Errors are normalized by half image size. Rates are rad/s; receiver must enforce TTL.

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct AlignmentCommand {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::rmw::Header,


    // This member is not documented.
    #[allow(missing_docs)]
    pub track_id: u64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub valid: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub predicted: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub error_x: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub error_y: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub yaw_rate: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub pitch_rate: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub aligned: bool,

    /// One-shot request per lock. External executor acknowledges by resetting the lock.
    pub grasp_request: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub valid_for_sec: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub reason: rosidl_runtime_rs::String,

}



impl Default for AlignmentCommand {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !block_interfaces__msg__AlignmentCommand__init(&mut msg as *mut _) {
        panic!("Call to block_interfaces__msg__AlignmentCommand__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for AlignmentCommand {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__AlignmentCommand__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__AlignmentCommand__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__AlignmentCommand__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for AlignmentCommand {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for AlignmentCommand where Self: Sized {
  const TYPE_NAME: &'static str = "block_interfaces/msg/AlignmentCommand";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__block_interfaces__msg__AlignmentCommand() }
  }
}


#[link(name = "block_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__block_interfaces__msg__GripperAngles() -> *const std::ffi::c_void;
}

#[link(name = "block_interfaces__rosidl_generator_c")]
extern "C" {
    fn block_interfaces__msg__GripperAngles__init(msg: *mut GripperAngles) -> bool;
    fn block_interfaces__msg__GripperAngles__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<GripperAngles>, size: usize) -> bool;
    fn block_interfaces__msg__GripperAngles__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<GripperAngles>);
    fn block_interfaces__msg__GripperAngles__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<GripperAngles>, out_seq: *mut rosidl_runtime_rs::Sequence<GripperAngles>) -> bool;
}

// Corresponds to block_interfaces__msg__GripperAngles
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// Perception proposal only, not a motor command. Angles are in radians.
/// Consumers MUST require angles_ready and reject expired acquisition timestamps.

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GripperAngles {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::rmw::Header,


    // This member is not documented.
    #[allow(missing_docs)]
    pub track_id: u64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub image_width: u32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub image_height: u32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub bounds_valid: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub left_edge_px: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub right_edge_px: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub angles_ready: bool,

    /// NaN until the calculation method is implemented; zero is NOT a fallback angle.
    pub left_angle_rad: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub right_angle_rad: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub valid_for_sec: f64,


    // This member is not documented.
    #[allow(missing_docs)]
    pub reason: rosidl_runtime_rs::String,

}



impl Default for GripperAngles {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !block_interfaces__msg__GripperAngles__init(&mut msg as *mut _) {
        panic!("Call to block_interfaces__msg__GripperAngles__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for GripperAngles {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__GripperAngles__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__GripperAngles__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { block_interfaces__msg__GripperAngles__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for GripperAngles {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for GripperAngles where Self: Sized {
  const TYPE_NAME: &'static str = "block_interfaces/msg/GripperAngles";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__block_interfaces__msg__GripperAngles() }
  }
}


