# generated from rosidl_generator_py/resource/_idl.py.em
# with input from block_interfaces:msg/AlignmentCommand.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import math  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_AlignmentCommand(type):
    """Metaclass of message 'AlignmentCommand'."""

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
                'block_interfaces.msg.AlignmentCommand')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__alignment_command
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__alignment_command
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__alignment_command
            cls._TYPE_SUPPORT = module.type_support_msg__msg__alignment_command
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__alignment_command

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


class AlignmentCommand(metaclass=Metaclass_AlignmentCommand):
    """Message class 'AlignmentCommand'."""

    __slots__ = [
        '_header',
        '_track_id',
        '_valid',
        '_predicted',
        '_error_x',
        '_error_y',
        '_yaw_rate',
        '_pitch_rate',
        '_aligned',
        '_grasp_request',
        '_valid_for_sec',
        '_reason',
    ]

    _fields_and_field_types = {
        'header': 'std_msgs/Header',
        'track_id': 'uint64',
        'valid': 'boolean',
        'predicted': 'boolean',
        'error_x': 'double',
        'error_y': 'double',
        'yaw_rate': 'double',
        'pitch_rate': 'double',
        'aligned': 'boolean',
        'grasp_request': 'boolean',
        'valid_for_sec': 'double',
        'reason': 'string',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.NamespacedType(['std_msgs', 'msg'], 'Header'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint64'),  # noqa: E501
        rosidl_parser.definition.BasicType('boolean'),  # noqa: E501
        rosidl_parser.definition.BasicType('boolean'),  # noqa: E501
        rosidl_parser.definition.BasicType('double'),  # noqa: E501
        rosidl_parser.definition.BasicType('double'),  # noqa: E501
        rosidl_parser.definition.BasicType('double'),  # noqa: E501
        rosidl_parser.definition.BasicType('double'),  # noqa: E501
        rosidl_parser.definition.BasicType('boolean'),  # noqa: E501
        rosidl_parser.definition.BasicType('boolean'),  # noqa: E501
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
        self.valid = kwargs.get('valid', bool())
        self.predicted = kwargs.get('predicted', bool())
        self.error_x = kwargs.get('error_x', float())
        self.error_y = kwargs.get('error_y', float())
        self.yaw_rate = kwargs.get('yaw_rate', float())
        self.pitch_rate = kwargs.get('pitch_rate', float())
        self.aligned = kwargs.get('aligned', bool())
        self.grasp_request = kwargs.get('grasp_request', bool())
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
        if self.valid != other.valid:
            return False
        if self.predicted != other.predicted:
            return False
        if self.error_x != other.error_x:
            return False
        if self.error_y != other.error_y:
            return False
        if self.yaw_rate != other.yaw_rate:
            return False
        if self.pitch_rate != other.pitch_rate:
            return False
        if self.aligned != other.aligned:
            return False
        if self.grasp_request != other.grasp_request:
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
    def valid(self):
        """Message field 'valid'."""
        return self._valid

    @valid.setter
    def valid(self, value):
        if __debug__:
            assert \
                isinstance(value, bool), \
                "The 'valid' field must be of type 'bool'"
        self._valid = value

    @builtins.property
    def predicted(self):
        """Message field 'predicted'."""
        return self._predicted

    @predicted.setter
    def predicted(self, value):
        if __debug__:
            assert \
                isinstance(value, bool), \
                "The 'predicted' field must be of type 'bool'"
        self._predicted = value

    @builtins.property
    def error_x(self):
        """Message field 'error_x'."""
        return self._error_x

    @error_x.setter
    def error_x(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'error_x' field must be of type 'float'"
            assert not (value < -1.7976931348623157e+308 or value > 1.7976931348623157e+308) or math.isinf(value), \
                "The 'error_x' field must be a double in [-1.7976931348623157e+308, 1.7976931348623157e+308]"
        self._error_x = value

    @builtins.property
    def error_y(self):
        """Message field 'error_y'."""
        return self._error_y

    @error_y.setter
    def error_y(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'error_y' field must be of type 'float'"
            assert not (value < -1.7976931348623157e+308 or value > 1.7976931348623157e+308) or math.isinf(value), \
                "The 'error_y' field must be a double in [-1.7976931348623157e+308, 1.7976931348623157e+308]"
        self._error_y = value

    @builtins.property
    def yaw_rate(self):
        """Message field 'yaw_rate'."""
        return self._yaw_rate

    @yaw_rate.setter
    def yaw_rate(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'yaw_rate' field must be of type 'float'"
            assert not (value < -1.7976931348623157e+308 or value > 1.7976931348623157e+308) or math.isinf(value), \
                "The 'yaw_rate' field must be a double in [-1.7976931348623157e+308, 1.7976931348623157e+308]"
        self._yaw_rate = value

    @builtins.property
    def pitch_rate(self):
        """Message field 'pitch_rate'."""
        return self._pitch_rate

    @pitch_rate.setter
    def pitch_rate(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'pitch_rate' field must be of type 'float'"
            assert not (value < -1.7976931348623157e+308 or value > 1.7976931348623157e+308) or math.isinf(value), \
                "The 'pitch_rate' field must be a double in [-1.7976931348623157e+308, 1.7976931348623157e+308]"
        self._pitch_rate = value

    @builtins.property
    def aligned(self):
        """Message field 'aligned'."""
        return self._aligned

    @aligned.setter
    def aligned(self, value):
        if __debug__:
            assert \
                isinstance(value, bool), \
                "The 'aligned' field must be of type 'bool'"
        self._aligned = value

    @builtins.property
    def grasp_request(self):
        """Message field 'grasp_request'."""
        return self._grasp_request

    @grasp_request.setter
    def grasp_request(self, value):
        if __debug__:
            assert \
                isinstance(value, bool), \
                "The 'grasp_request' field must be of type 'bool'"
        self._grasp_request = value

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
