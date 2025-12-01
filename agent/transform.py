"""
Transform 类 - CARLA 风格的位置和旋转封装
"""
from typing import List, Optional


class Location:
    """位置类"""

    def __init__(self, x: float = 0, y: float = 0, z: float = 0):
        self.x = x
        self.y = y
        self.z = z

    def __repr__(self):
        return f"Location(x={self.x}, y={self.y}, z={self.z})"

    def to_list(self) -> List[float]:
        return [self.x, self.y, self.z]

    @classmethod
    def from_list(cls, data: List[float]) -> "Location":
        return cls(x=data[0], y=data[1], z=data[2])


class Rotation:
    """旋转类 (pitch, yaw, roll)"""

    def __init__(self, pitch: float = 0, yaw: float = 0, roll: float = 0):
        self.pitch = pitch
        self.yaw = yaw
        self.roll = roll

    def __repr__(self):
        return f"Rotation(pitch={self.pitch}, yaw={self.yaw}, roll={self.roll})"

    def to_list(self) -> List[float]:
        return [self.pitch, self.yaw, self.roll]

    @classmethod
    def from_list(cls, data: List[float]) -> "Rotation":
        return cls(pitch=data[0], yaw=data[1], roll=data[2])


class Scale:
    """缩放类"""

    def __init__(self, x: float = 1, y: float = 1, z: float = 1):
        self.x = x
        self.y = y
        self.z = z

    def __repr__(self):
        return f"Scale(x={self.x}, y={self.y}, z={self.z})"

    def to_list(self) -> List[float]:
        return [self.x, self.y, self.z]

    @classmethod
    def from_list(cls, data: List[float]) -> "Scale":
        return cls(x=data[0], y=data[1], z=data[2])


class Transform:
    """变换类，包含位置和旋转"""

    def __init__(
        self,
        location: Optional[Location] = None,
        rotation: Optional[Rotation] = None,
        scale: Optional[Scale] = None,
    ):
        self.location = location or Location()
        self.rotation = rotation or Rotation()
        self.scale = scale or Scale()

    def __repr__(self):
        return f"Transform({self.location}, {self.rotation}, {self.scale})"
