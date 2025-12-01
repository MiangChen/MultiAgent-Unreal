from typing import Callable, Optional

import numpy as np

from .sensor import Sensor


class SensorRgb(Sensor):
    """
    摄像头传感器
    """

    def __init__(self, client, sensor_id: int, sensor_type: str = "camera.rgb"):
        super().__init__(client, sensor_id, sensor_type)

    def get_image(self, mode: str = "lit", format: str = "bmp") -> Optional[np.ndarray]:
        """
        获取图像

        Args:
            mode: 图像模式 ('lit', 'depth', 'object_mask', 'normal')
            format: 图像格式 ('bmp', 'png', 'npy')

        Returns:
            图像数组
        """
        if not self._is_alive:
            return None
        return self.client.get_image(self.sensor_id, mode, format)

    def get_depth(self) -> Optional[np.ndarray]:
        """获取深度图"""
        if not self._is_alive:
            return None
        return self.client.get_depth(self.sensor_id)

    def get_location(self):
        """获取摄像头位置"""
        return self.client.get_cam_location(self.sensor_id)

    def get_rotation(self):
        """获取摄像头旋转"""
        return self.client.get_cam_rotation(self.sensor_id)

    def set_rotation(self, pitch: float, yaw: float, roll: float) -> None:
        """设置摄像头旋转"""
        cmd = f"vset /camera/{self.sensor_id}/rotation {pitch} {yaw} {roll}"
        self.client.client.request(cmd)

    def set_fov(self, fov: float) -> None:
        """设置视场角"""
        self.client.set_cam_fov(self.sensor_id, fov)

    def get_fov(self) -> float:
        """获取视场角"""
        return self.client.get_cam_fov(self.sensor_id)

    def listen(self, callback: Callable[[np.ndarray], None]) -> None:
        """
        注册回调函数（注意：UnrealCV 不支持真正的异步回调，
        这里只是保存回调，需要手动调用 tick() 触发）

        Args:
            callback: 回调函数，接收图像数组
        """
        self._callback = callback

    def tick(self) -> None:
        """触发一次数据采集并调用回调"""
        if self._callback and self._is_alive:
            image = self.get_image()
            if image is not None:
                self._callback(image)
