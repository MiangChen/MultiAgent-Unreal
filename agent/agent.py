"""
Agent 类 - 封装 UE 中的智能体对象
"""
from typing import TYPE_CHECKING, List, Optional

import numpy as np

from agent.transform import Location, Rotation, Scale, Transform

if TYPE_CHECKING:
    from agent.sensor import Sensor


class Agent:
    """
    Agent 类，封装 UE 中的智能体操作
    """

    def __init__(
        self,
        client,
        name: str,
        class_name: str,
        transform: Transform,
        cam_id: int = -1,
    ):
        """
        初始化 Agent

        Args:
            client: UnrealCv_API 客户端
            name: 对象名称
            class_name: 蓝图类名
            transform: 变换信息
            cam_id: 绑定的摄像头 ID，-1 表示无摄像头
        """
        self.client = client
        self.name = name
        self.class_name = class_name
        self.transform = transform
        self.cam_id = cam_id
        self._is_alive = True
        self._sensors: List["Sensor"] = []

    def get_location(self) -> Location:
        """获取当前位置"""
        loc = self.client.get_obj_location(self.name)
        return Location.from_list(loc)

    def set_location(self, location: Location) -> None:
        """设置位置"""
        x, y, z = location.to_list()
        cmd = f"vset /object/{self.name}/location {x} {y} {z}"
        self.client.client.request(cmd)
        self.transform.location = location

    def get_rotation(self) -> Rotation:
        """获取当前旋转"""
        rot = self.client.get_obj_rotation(self.name)
        return Rotation.from_list(rot)

    def set_rotation(self, rotation: Rotation) -> None:
        """设置旋转"""
        pitch, yaw, roll = rotation.to_list()
        cmd = f"vset /object/{self.name}/rotation {pitch} {yaw} {roll}"
        self.client.client.request(cmd)
        self.transform.rotation = rotation

    def set_scale(self, scale: Scale) -> None:
        """设置缩放"""
        sx, sy, sz = scale.to_list()
        cmd = f"vset /object/{self.name}/scale {sx} {sy} {sz}"
        self.client.client.request(cmd)
        self.transform.scale = scale

    def get_transform(self) -> Transform:
        """获取完整变换"""
        return Transform(
            location=self.get_location(),
            rotation=self.get_rotation(),
            scale=self.transform.scale,
        )

    def destroy(self) -> bool:
        """销毁 Agent 及其所有传感器"""
        if not self._is_alive:
            return False

        # 销毁所有传感器
        for sensor in self._sensors[:]:
            sensor.destroy()

        cmd = f"vset /object/{self.name}/destroy"
        self.client.client.request(cmd)
        self._is_alive = False
        print(f"   🗑️ Destroyed: {self.name}")
        return True

    def is_alive(self) -> bool:
        """检查 Agent 是否存活"""
        return self._is_alive

    # ==================== 传感器相关方法 ====================

    def get_sensors(self) -> List["Sensor"]:
        """获取所有绑定的传感器"""
        return self._sensors

    def get_sensor(self, sensor_type: str = "camera.rgb") -> Optional["Sensor"]:
        """获取指定类型的第一个传感器"""
        for sensor in self._sensors:
            if sensor.sensor_type == sensor_type:
                return sensor
        return None

    def has_sensor(self, sensor_type: str = None) -> bool:
        """检查是否有传感器"""
        if sensor_type is None:
            return len(self._sensors) > 0
        return any(s.sensor_type == sensor_type for s in self._sensors)

    # ==================== 便捷方法（通过默认传感器）====================

    def get_image(self, mode: str = "lit", format: str = "bmp") -> Optional[np.ndarray]:
        """获取默认摄像头图像"""
        sensor = self.get_sensor("camera.rgb")
        if sensor:
            return sensor.get_image(mode, format)
        return None

    def get_depth(self) -> Optional[np.ndarray]:
        """获取深度图"""
        sensor = self.get_sensor("camera.rgb") or self.get_sensor("camera.depth")
        if sensor:
            return sensor.get_depth()
        return None

    def __repr__(self):
        sensor_info = f", sensors={len(self._sensors)}" if self._sensors else ""
        return f"Agent(name='{self.name}', class='{self.class_name}'{sensor_info})"
