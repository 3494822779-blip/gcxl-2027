# generated from rosidl_generator_py/resource/_idl.py.em
# with input from block_interfaces:msg/GripperAngles.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import math  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_GripperAngles(type):
    """Metaclass of message 'GripperAngles'."""

    _CREATE_ROS_MESSAGE = None
    _CONVERT_FROM_PY = None
    _CONVERT_TO_PY = None
    _DESTROY_ROS_MESSAGE = None
    _TYPE_SUPPORT = None

    __constants = {
    }

    @classmethod
    def __import_type_support__(cls):
        try:
            from rosidl_generator_py import import_type_support
            module = import_type_support('block_interfaces')
        except ImportError:
            import logging
            import traceback
            logger = logging.getLogger(
                'block_interfaces.msg.GripperAngles')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__gripper_angles
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__gripper_angles
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__gripper_angles
            cls._TYPE_SUPPORT = module.type_support_msg__msg__gripper_angles
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__gripper_angles

            from std_msgs.msg import Header
            if Header.__class__._TYPE_SUPPORT is None:
                Header.__class__.__import_type_support__()

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class GripperAngles(metaclass=Metaclass_GripperAngles):
    """Message class 'GripperAngles'."""

    __slots__ = [
        '_header',
        '_track_id',
        '_image_width',
        '_image_height',
        '_bounds_valid',
        '_left_edge_px',
        '_right_edge_px',
        '_angles_ready',
        '_left_angle_rad',
        '_right_angle_rad',
        '_valid_for_sec',
        '_reason',
    ]

    _fields_and_field_types = {
        'header': 'std_msgs/Header',
        'track_id': 'uint64',
        'image_width': 'uint32',
        'image_height': 'uint32',
        'bounds_valid': 'boolean',
        'left_edge_px': 'double',
        'right_edge_px': 'double',
        'angles_ready': 'boolean',
        'left_angle_rad': 'double',
        'right_angle_rad': 'double',
        'valid_for_sec': 'double',
        'reason': 'string',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.NamespacedType(['std_msgs', 'msg'], 'Header'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint64'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint32'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint32'),  # noqa: E501
        rosidl_parser.definition.BasicType('boolean'),  # noqa: E501
        rosidl_parser.definition.BasicType('double'),  # noqa: E501
        rosidl_parser.definition.BasicType('double'),  # noqa: E501
        rosidl_parser.definition.BasicType('boolean'),  # noqa: E501
        rosidl_parser.definition.BasicType('double'),  # noqa: E501
        rosidl_parser.definition.BasicType('double'),  # noqa: E501
        rosidl_parser.definition.BasicType('double'),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        from std_msgs.msg import Header
        self.header = kwargs.get('header', Header())
        self.track_id = kwargs.get('track_id', int())
        self.image_width = kwargs.get('image_width', int())
        self.image_height = kwargs.get('image_height', int())
        self.bounds_valid = kwargs.get('bounds_valid', bool())
        self.left_edge_px = kwargs.get('left_edge_px', float())
        self.right_edge_px = kwargs.get('right_edge_px', float())
        self.angles_ready = kwargs.get('angles_ready', bool())
        self.left_angle_rad = kwargs.get('left_angle_rad', float())
        self.right_angle_rad = kwargs.get('right_angle_rad', float())
        self.valid_for_sec = kwargs.get('valid_for_sec', float())
        self.reason = kwargs.get('reason', str())

    def __repr__(self):
        typename = self.__class__.__module__.split('.')
        typename.pop()
        typename.append(self.__class__.__name__)
        args = []
        for s, t in zip(self.__slots__, self.SLOT_TYPES):
            field = getattr(self, s)
            fieldstr = repr(field)
            # We use Python array type for fields that can be directly stored
            # in them, and "normal" sequences for everything else.  If it is
            # a type that we store in an array, strip off the 'array' portion.
            if (
                isinstance(t, rosidl_parser.definition.AbstractSequence) and
                isinstance(t.value_type, rosidl_parser.definition.BasicType) and
                t.value_type.typename in ['float', 'double', 'int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32', 'int64', 'uint64']
            ):
                if len(field) == 0:
                    fieldstr = '[]'
                else:
                    assert fieldstr.startswith('array(')
                    prefix = "array('X', "
                    suffix = ')'
                    fieldstr = fieldstr[len(prefix):-len(suffix)]
            args.append(s[1:] + '=' + fieldstr)
        return '%s(%s)' % ('.'.join(typename), ', '.join(args))

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return False
        if self.header != other.header:
            return False
        if self.track_id != other.track_id:
            return False
        if self.image_width != other.image_width:
            return False
        if self.image_height != other.image_height:
            return False
        if self.bounds_valid != other.bounds_valid:
            return False
        if self.left_edge_px != other.left_edge_px:
            return False
        if self.right_edge_px != other.right_edge_px:
            return False
        if self.angles_ready != other.angles_ready:
            return False
        if self.left_angle_rad != other.left_angle_rad:
            return False
        if self.right_angle_rad != other.right_angle_rad:
            return False
        if self.valid_for_sec != other.valid_for_sec:
            return False
        if self.reason != other.reason:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def header(self):
        """Message field 'header'."""
        return self._header

    @header.setter
    def header(self, value):
        if __debug__:
            from std_msgs.msg import Header
            assert \
                isinstance(value, Header), \
                "The 'header' field must be a sub message of type 'Header'"
        self._header = value

    @builtins.property
    def track_id(self):
        """Message field 'track_id'."""
        return self._track_id

    @track_id.setter
    def track_id(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'track_id' field must be of type 'int'"
            assert value >= 0 and value < 18446744073709551616, \
                "The 'track_id' field must be an unsigned integer in [0, 18446744073709551615]"
        self._track_id = value

    @builtins.property
    def image_width(self):
        """Message field 'image_width'."""
        return self._image_width

    @image_width.setter
    def image_width(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'image_width' field must be of type 'int'"
            assert value >= 0 and value < 4294967296, \
                "The 'image_width' field must be an unsigned integer in [0, 4294967295]"
        self._image_width = value

    @builtins.property
    def image_height(self):
        """Message field 'image_height'."""
        return self._image_height

    @image_height.setter
    def image_height(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'image_height' field must be of type 'int'"
            assert value >= 0 and value < 4294967296, \
                "The 'image_height' field must be an unsigned integer in [0, 4294967295]"
        self._image_height = value

    @builtins.property
    def bounds_valid(self):
        """Message field 'bounds_valid'."""
        return self._bounds_valid

    @bounds_valid.setter
    def bounds_valid(self, value):
        if __debug__:
            assert \
                isinstance(value, bool), \
                "The 'bounds_valid' field must be of type 'bool'"
        self._bounds_valid = value

    @builtins.property
    def left_edge_px(self):
        """Message field 'left_edge_px'."""
        return self._left_edge_px

    @left_edge_px.setter
    def left_edge_px(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'left_edge_px' field must be of type 'float'"
            assert not (value < -1.7976931348623157e+308 or value > 1.7976931348623157e+308) or math.isinf(value), \
                "The 'left_edge_px' field must be a double in [-1.7976931348623157e+308, 1.7976931348623157e+308]"
        self._left_edge_px = value

    @builtins.property
    def right_edge_px(self):
        """Message field 'right_edge_px'."""
        return self._right_edge_px

    @right_edge_px.setter
    def right_edge_px(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'right_edge_px' field must be of type 'float'"
            assert not (value < -1.7976931348623157e+308 or value > 1.7976931348623157e+308) or math.isinf(value), \
                "The 'right_edge_px' field must be a double in [-1.7976931348623157e+308, 1.7976931348623157e+308]"
        self._right_edge_px = value

    @builtins.property
    def angles_ready(self):
        """Message field 'angles_ready'."""
        return self._angles_ready

    @angles_ready.setter
    def angles_ready(self, value):
        if __debug__:
            assert \
                isinstance(value, bool), \
                "The 'angles_ready' field must be of type 'bool'"
        self._angles_ready = value

    @builtins.property
    def left_angle_rad(self):
        """Message field 'left_angle_rad'."""
        return self._left_angle_rad

    @left_angle_rad.setter
    def left_angle_rad(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'left_angle_rad' field must be of type 'float'"
            assert not (value < -1.7976931348623157e+308 or value > 1.7976931348623157e+308) or math.isinf(value), \
                "The 'left_angle_rad' field must be a double in [-1.7976931348623157e+308, 1.7976931348623157e+308]"
        self._left_angle_rad = value

    @builtins.property
    def right_angle_rad(self):
        """Message field 'right_angle_rad'."""
        return self._right_angle_rad

    @right_angle_rad.setter
    def right_angle_rad(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'right_angle_rad' field must be of type 'float'"
            assert not (value < -1.7976931348623157e+308 or value > 1.7976931348623157e+308) or math.isinf(value), \
                "The 'right_angle_rad' field must be a double in [-1.7976931348623157e+308, 1.7976931348623157e+308]"
        self._right_angle_rad = value

    @builtins.property
    def valid_for_sec(self):
        """Message field 'valid_for_sec'."""
        return self._valid_for_sec

    @valid_for_sec.setter
    def valid_for_sec(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'valid_for_sec' field must be of type 'float'"
            assert not (value < -1.7976931348623157e+308 or value > 1.7976931348623157e+308) or math.isinf(value), \
                "The 'valid_for_sec' field must be a double in [-1.7976931348623157e+308, 1.7976931348623157e+308]"
        self._valid_for_sec = value

    @builtins.property
    def reason(self):
        """Message field 'reason'."""
        return self._reason

    @reason.setter
    def reason(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'reason' field must be of type 'str'"
        self._reason = value
