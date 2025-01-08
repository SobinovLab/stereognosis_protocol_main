from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from typing import ClassVar as _ClassVar, Optional as _Optional

DESCRIPTOR: _descriptor.FileDescriptor

class readyRequest(_message.Message):
    __slots__ = ()
    def __init__(self) -> None: ...

class readyResponse(_message.Message):
    __slots__ = ("flag",)
    FLAG_FIELD_NUMBER: _ClassVar[int]
    flag: int
    def __init__(self, flag: _Optional[int] = ...) -> None: ...

class statusRequest(_message.Message):
    __slots__ = ("flag",)
    FLAG_FIELD_NUMBER: _ClassVar[int]
    flag: int
    def __init__(self, flag: _Optional[int] = ...) -> None: ...

class statusResponse(_message.Message):
    __slots__ = ("flag",)
    FLAG_FIELD_NUMBER: _ClassVar[int]
    flag: int
    def __init__(self, flag: _Optional[int] = ...) -> None: ...

class gripperRequest(_message.Message):
    __slots__ = ("width",)
    WIDTH_FIELD_NUMBER: _ClassVar[int]
    width: float
    def __init__(self, width: _Optional[float] = ...) -> None: ...

class stopRequest(_message.Message):
    __slots__ = ()
    def __init__(self) -> None: ...

class moveArm(_message.Message):
    __slots__ = ("lateral", "depth", "height", "theta", "phi", "chi", "width")
    LATERAL_FIELD_NUMBER: _ClassVar[int]
    DEPTH_FIELD_NUMBER: _ClassVar[int]
    HEIGHT_FIELD_NUMBER: _ClassVar[int]
    THETA_FIELD_NUMBER: _ClassVar[int]
    PHI_FIELD_NUMBER: _ClassVar[int]
    CHI_FIELD_NUMBER: _ClassVar[int]
    WIDTH_FIELD_NUMBER: _ClassVar[int]
    lateral: float
    depth: float
    height: float
    theta: float
    phi: float
    chi: float
    width: float
    def __init__(self, lateral: _Optional[float] = ..., depth: _Optional[float] = ..., height: _Optional[float] = ..., theta: _Optional[float] = ..., phi: _Optional[float] = ..., chi: _Optional[float] = ..., width: _Optional[float] = ...) -> None: ...

class moveHome(_message.Message):
    __slots__ = ("flag",)
    FLAG_FIELD_NUMBER: _ClassVar[int]
    flag: int
    def __init__(self, flag: _Optional[int] = ...) -> None: ...

class moveResponse(_message.Message):
    __slots__ = ("responseCode",)
    RESPONSECODE_FIELD_NUMBER: _ClassVar[int]
    responseCode: int
    def __init__(self, responseCode: _Optional[int] = ...) -> None: ...

class rotateRequest(_message.Message):
    __slots__ = ("speed", "duration")
    SPEED_FIELD_NUMBER: _ClassVar[int]
    DURATION_FIELD_NUMBER: _ClassVar[int]
    speed: float
    duration: float
    def __init__(self, speed: _Optional[float] = ..., duration: _Optional[float] = ...) -> None: ...

class rotateResponse(_message.Message):
    __slots__ = ("angle", "flag")
    ANGLE_FIELD_NUMBER: _ClassVar[int]
    FLAG_FIELD_NUMBER: _ClassVar[int]
    angle: float
    flag: int
    def __init__(self, angle: _Optional[float] = ..., flag: _Optional[int] = ...) -> None: ...

class torqueRequest(_message.Message):
    __slots__ = ("flag",)
    FLAG_FIELD_NUMBER: _ClassVar[int]
    flag: int
    def __init__(self, flag: _Optional[int] = ...) -> None: ...

class torqueResponse(_message.Message):
    __slots__ = ("joint1", "joint2", "joint3", "joint4", "joint5", "joint6", "endpoint")
    JOINT1_FIELD_NUMBER: _ClassVar[int]
    JOINT2_FIELD_NUMBER: _ClassVar[int]
    JOINT3_FIELD_NUMBER: _ClassVar[int]
    JOINT4_FIELD_NUMBER: _ClassVar[int]
    JOINT5_FIELD_NUMBER: _ClassVar[int]
    JOINT6_FIELD_NUMBER: _ClassVar[int]
    ENDPOINT_FIELD_NUMBER: _ClassVar[int]
    joint1: float
    joint2: float
    joint3: float
    joint4: float
    joint5: float
    joint6: float
    endpoint: float
    def __init__(self, joint1: _Optional[float] = ..., joint2: _Optional[float] = ..., joint3: _Optional[float] = ..., joint4: _Optional[float] = ..., joint5: _Optional[float] = ..., joint6: _Optional[float] = ..., endpoint: _Optional[float] = ...) -> None: ...
