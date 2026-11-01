#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};



// Corresponds to block_interfaces__msg__Box

// This struct is not documented.
#[allow(missing_docs)]

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
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
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::Box::default())
  }
}

impl rosidl_runtime_rs::Message for Box {
  type RmwMsg = super::msg::rmw::Box;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        track_id: msg.track_id,
        class_id: msg.class_id,
        confidence: msg.confidence,
        x: msg.x,
        y: msg.y,
        width: msg.width,
        height: msg.height,
        observed: msg.observed,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      track_id: msg.track_id,
      class_id: msg.class_id,
      confidence: msg.confidence,
      x: msg.x,
      y: msg.y,
      width: msg.width,
      height: msg.height,
      observed: msg.observed,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      track_id: msg.track_id,
      class_id: msg.class_id,
      confidence: msg.confidence,
      x: msg.x,
      y: msg.y,
      width: msg.width,
      height: msg.height,
      observed: msg.observed,
    }
  }
}


// Corresponds to block_interfaces__msg__Frame
/// Image and boxes share exactly the same acquisition timestamp and pixel coordinates.

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct Frame {

    // This member is not documented.
    #[allow(missing_docs)]
    pub image: sensor_msgs::msg::Image,


    // This member is not documented.
    #[allow(missing_docs)]
    pub boxes: Vec<super::msg::Box>,

}



impl Default for Frame {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::Frame::default())
  }
}

impl rosidl_runtime_rs::Message for Frame {
  type RmwMsg = super::msg::rmw::Frame;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        image: sensor_msgs::msg::Image::into_rmw_message(std::borrow::Cow::Owned(msg.image)).into_owned(),
        boxes: msg.boxes
          .into_iter()
          .map(|elem| super::msg::Box::into_rmw_message(std::borrow::Cow::Owned(elem)).into_owned())
          .collect(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        image: sensor_msgs::msg::Image::into_rmw_message(std::borrow::Cow::Borrowed(&msg.image)).into_owned(),
        boxes: msg.boxes
          .iter()
          .map(|elem| super::msg::Box::into_rmw_message(std::borrow::Cow::Borrowed(elem)).into_owned())
          .collect(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      image: sensor_msgs::msg::Image::from_rmw_message(msg.image),
      boxes: msg.boxes
          .into_iter()
          .map(super::msg::Box::from_rmw_message)
          .collect(),
    }
  }
}


// Corresponds to block_interfaces__msg__Target

// This struct is not documented.
#[allow(missing_docs)]

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct Target {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::Header,


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
    pub last_observed: builtin_interfaces::msg::Time,

}



impl Default for Target {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::Target::default())
  }
}

impl rosidl_runtime_rs::Message for Target {
  type RmwMsg = super::msg::rmw::Target;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Owned(msg.header)).into_owned(),
        image_width: msg.image_width,
        image_height: msg.image_height,
        locked: msg.locked,
        observed: msg.observed,
        valid: msg.valid,
        track_id: msg.track_id,
        class_id: msg.class_id,
        raw_x: msg.raw_x,
        raw_y: msg.raw_y,
        x: msg.x,
        y: msg.y,
        last_observed: builtin_interfaces::msg::Time::into_rmw_message(std::borrow::Cow::Owned(msg.last_observed)).into_owned(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Borrowed(&msg.header)).into_owned(),
      image_width: msg.image_width,
      image_height: msg.image_height,
      locked: msg.locked,
      observed: msg.observed,
      valid: msg.valid,
      track_id: msg.track_id,
      class_id: msg.class_id,
      raw_x: msg.raw_x,
      raw_y: msg.raw_y,
      x: msg.x,
      y: msg.y,
        last_observed: builtin_interfaces::msg::Time::into_rmw_message(std::borrow::Cow::Borrowed(&msg.last_observed)).into_owned(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      header: std_msgs::msg::Header::from_rmw_message(msg.header),
      image_width: msg.image_width,
      image_height: msg.image_height,
      locked: msg.locked,
      observed: msg.observed,
      valid: msg.valid,
      track_id: msg.track_id,
      class_id: msg.class_id,
      raw_x: msg.raw_x,
      raw_y: msg.raw_y,
      x: msg.x,
      y: msg.y,
      last_observed: builtin_interfaces::msg::Time::from_rmw_message(msg.last_observed),
    }
  }
}


// Corresponds to block_interfaces__msg__AlignmentCommand
/// Errors are normalized by half image size. Rates are rad/s; receiver must enforce TTL.

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct AlignmentCommand {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::Header,


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
    pub reason: std::string::String,

}



impl Default for AlignmentCommand {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::AlignmentCommand::default())
  }
}

impl rosidl_runtime_rs::Message for AlignmentCommand {
  type RmwMsg = super::msg::rmw::AlignmentCommand;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Owned(msg.header)).into_owned(),
        track_id: msg.track_id,
        valid: msg.valid,
        predicted: msg.predicted,
        error_x: msg.error_x,
        error_y: msg.error_y,
        yaw_rate: msg.yaw_rate,
        pitch_rate: msg.pitch_rate,
        aligned: msg.aligned,
        grasp_request: msg.grasp_request,
        valid_for_sec: msg.valid_for_sec,
        reason: msg.reason.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Borrowed(&msg.header)).into_owned(),
      track_id: msg.track_id,
      valid: msg.valid,
      predicted: msg.predicted,
      error_x: msg.error_x,
      error_y: msg.error_y,
      yaw_rate: msg.yaw_rate,
      pitch_rate: msg.pitch_rate,
      aligned: msg.aligned,
      grasp_request: msg.grasp_request,
      valid_for_sec: msg.valid_for_sec,
        reason: msg.reason.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      header: std_msgs::msg::Header::from_rmw_message(msg.header),
      track_id: msg.track_id,
      valid: msg.valid,
      predicted: msg.predicted,
      error_x: msg.error_x,
      error_y: msg.error_y,
      yaw_rate: msg.yaw_rate,
      pitch_rate: msg.pitch_rate,
      aligned: msg.aligned,
      grasp_request: msg.grasp_request,
      valid_for_sec: msg.valid_for_sec,
      reason: msg.reason.to_string(),
    }
  }
}


// Corresponds to block_interfaces__msg__GripperAngles
/// Perception proposal only, not a motor command. Angles are in radians.
/// Consumers MUST require angles_ready and reject expired acquisition timestamps.

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct GripperAngles {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::Header,


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
    pub reason: std::string::String,

}



impl Default for GripperAngles {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::GripperAngles::default())
  }
}

impl rosidl_runtime_rs::Message for GripperAngles {
  type RmwMsg = super::msg::rmw::GripperAngles;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Owned(msg.header)).into_owned(),
        track_id: msg.track_id,
        image_width: msg.image_width,
        image_height: msg.image_height,
        bounds_valid: msg.bounds_valid,
        left_edge_px: msg.left_edge_px,
        right_edge_px: msg.right_edge_px,
        angles_ready: msg.angles_ready,
        left_angle_rad: msg.left_angle_rad,
        right_angle_rad: msg.right_angle_rad,
        valid_for_sec: msg.valid_for_sec,
        reason: msg.reason.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Borrowed(&msg.header)).into_owned(),
      track_id: msg.track_id,
      image_width: msg.image_width,
      image_height: msg.image_height,
      bounds_valid: msg.bounds_valid,
      left_edge_px: msg.left_edge_px,
      right_edge_px: msg.right_edge_px,
      angles_ready: msg.angles_ready,
      left_angle_rad: msg.left_angle_rad,
      right_angle_rad: msg.right_angle_rad,
      valid_for_sec: msg.valid_for_sec,
        reason: msg.reason.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      header: std_msgs::msg::Header::from_rmw_message(msg.header),
      track_id: msg.track_id,
      image_width: msg.image_width,
      image_height: msg.image_height,
      bounds_valid: msg.bounds_valid,
      left_edge_px: msg.left_edge_px,
      right_edge_px: msg.right_edge_px,
      angles_ready: msg.angles_ready,
      left_angle_rad: msg.left_angle_rad,
      right_angle_rad: msg.right_angle_rad,
      valid_for_sec: msg.valid_for_sec,
      reason: msg.reason.to_string(),
    }
  }
}


