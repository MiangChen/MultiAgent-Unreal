"""
Sensor 类 - 封装 UE 中的传感器（摄像头等）
"""
from typing import Callable, Optional, TYPE_CHECKING

if TYPE_CHECKING:
    from agent.agent import Agent


class Sensor:
    """
    传感器基类
    """

    def __init__(self, client, sensor_id: int, sensor_type: str = "camera.rgb"):
        """
        初始化传感器

        Args:
            client: UnrealCv_API 客户端
            sensor_id: 传感器 ID（对应 UE 中的 cam_id）
            sensor_type: 传感器类型
        """
        self.client = client
        self.sensor_id = sensor_id
        self.sensor_type = sensor_type
        self.attached_to: Optional["Agent"] = None
        self._is_alive = True
        self._callback: Optional[Callable] = None

    def attach_to(self, agent: "Agent") -> None:
        """绑定到 Agent"""
        self.attached_to = agent
        agent._sensors.append(self)

    def detach(self) -> None:
        """从 Agent 解绑"""
        if self.attached_to:
            self.attached_to._sensors.remove(self)
            self.attached_to = None

    def is_alive(self) -> bool:
        """检查传感器是否存活"""
        return self._is_alive

    def destroy(self) -> None:
        """销毁传感器"""
        self.detach()
        self._is_alive = False

    def __repr__(self):
        attached = f", attached_to='{self.attached_to.name}'" if self.attached_to else ""
        return f"Sensor(id={self.sensor_id}, type='{self.sensor_type}'{attached})"
