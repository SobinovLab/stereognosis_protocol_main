from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from typing import ClassVar as _ClassVar, Optional as _Optional

DESCRIPTOR: _descriptor.FileDescriptor

class gripperRequest(_message.Message):
    __slots__ = ["width"]
    WIDTH_FIELD_NUMBER: _ClassVar[int]
    width: float
    def __init__(self, width: _Optional[float] = ...) -> None: ...

class moveArm(_message.Message):
    __slots__ = ["chi", "depth", "height", "lateral", "phi", "theta", "width"]
    CHI_FIELD_NUMBER: _ClassVar[int]
    DEPTH_FIELD_NUMBER: _ClassVar[int]
    HEIGHT_FIELD_NUMBER: _ClassVar[int]
    LATERAL_FIELD_NUMBER: _ClassVar[int]
    PHI_FIELD_NUMBER: _ClassVar[int]
    THETA_FIELD_NUMBER: _ClassVar[int]
    WIDTH_FIELD_NUMBER: _ClassVar[int]
    chi: float
    depth: float
    height: float
    lateral: float
    phi: float
    theta: float
    width: float
    def __init__(self, lateral: _Optional[float] = ..., depth: _Optional[float] = ..., height: _Optional[float] = ..., theta: _Optional[float] = ..., phi: _Optional[float] = ..., chi: _Optional[float] = ..., width: _Optional[float] = ...) -> None: ...

class moveHome(_message.Message):
    __slots__ = ["flag"]
    FLAG_FIELD_NUMBER: _ClassVar[int]
    flag: int
    def __init__(self, flag: _Optional[int] = ...) -> None: ...

class moveResponse(_message.Message):
    __slots__ = ["responseCode"]
    RESPONSECODE_FIELD_NUMBER: _ClassVar[int]
    responseCode: int
    def __init__(self, responseCode: _Optional[int] = ...) -> None: ...

class statusRequest(_message.Message):
    __slots__ = ["flag"]
    FLAG_FIELD_NUMBER: _ClassVar[int]
    flag: int
    def __init__(self, flag: _Optional[int] = ...) -> None: ...

class statusResponse(_message.Message):
    __slots__ = ["flag"]
    FLAG_FIELD_NUMBER: _ClassVar[int]
    flag: int
    def __init__(self, flag: _Optional[int] = ...) -> None: ...

class stopRequest(_message.Message):
    __slots__ = []
    def __init__(self) -> None: ...

class torqueRequest(_message.Message):
    __slots__ = ["flag"]
    FLAG_FIELD_NUMBER: _ClassVar[int]
    flag: int
    def __init__(self, flag: _Optional[int] = ...) -> None: ...

class torqueResponse(_message.Message):
    __slots__ = ["endpoint", "joint1", "joint2", "joint3", "joint4", "joint5", "joint6"]
    ENDPOINT_FIELD_NUMBER: _ClassVar[int]
    JOINT1_FIELD_NUMBER: _ClassVar[int]
    JOINT2_FIELD_NUMBER: _ClassVar[int]
    JOINT3_FIELD_NUMBER: _ClassVar[int]
    JOINT4_FIELD_NUMBER: _ClassVar[int]
    JOINT5_FIELD_NUMBER: _ClassVar[int]
    JOINT6_FIELD_NUMBER: _ClassVar[int]
    endpoint: float
    joint1: float
    joint2: float
    joint3: float
    joint4: float
    joint5: float
    joint6: float
    def __init__(self, joint1: _Optional[float] = ..., joint2: _Optional[float] = ..., joint3: _Optional[float] = ..., joint4: _Optional[float] = ..., joint5: _Optional[float] = ..., joint6: _Optional[float] = ..., endpoint: _Optional[float] = ...) -> None: ...
