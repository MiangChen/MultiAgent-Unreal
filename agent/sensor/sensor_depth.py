from typing import Optional

import numpy as np

from .sensor import Sensor


class SensorDepth(Sensor):
    """深度传感器"""

    def __init__(self, client, sensor_id: int):
        super().__init__(client, sensor_id, "camera.depth")

    def get_image(self, mode: str = "depth", format: str = "npy") -> Optional[np.ndarray]:
        """获取深度图像"""
        if not self._is_alive:
            return None
        return self.client.get_image(self.sensor_id, mode, format)

    def get_depth(self) -> Optional[np.ndarray]:
        """获取深度数据"""
        if not self._is_alive:
            return None
        try:
            return self.client.get_depth(self.sensor_id)
        except (TypeError, Exception) as e:
            print(f"   ⚠️ get_depth failed for sensor {self.sensor_id}: {e}")
            # 回退到 get_image 方式获取深度
            return self.get_image(mode="depth", format="npy")

    def get_location(self):
        """获取传感器位置"""
        return self.client.get_cam_location(self.sensor_id)

    def get_rotation(self):
        """获取传感器旋转"""
        return self.client.get_cam_rotation(self.sensor_id)
