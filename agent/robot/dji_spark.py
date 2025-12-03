"""
DJISpark 类 - 封装 UE 中的 DJI Spark 无人机操作

DJI Spark 是一款小型无人机，朝向固定为 0，不支持旋转控制。
资源路径: /Game/Robot/dji_spark/DJI_Spark_Fly
动画资源: /Game/Robot/dji_spark/DJI_Spark_Fly_Anim
"""
import math
import time
from typing import TYPE_CHECKING

from agent.transform import Location, Rotation

if TYPE_CHECKING:
    from agent.agent import Agent


class DJISpark:
    """
    DJI Spark 无人机类，封装 Spark 特有的操作（飞行、悬停、速度控制等）
    
    与 Drone 类的主要区别：
    - 朝向固定为 0，不支持 yaw 旋转
    - 速度控制直接使用世界坐标系（无需坐标变换）
    
    资源信息:
    - 蓝图类: BP_DJISpark_C (需要在 UE 中创建)
    - 骨骼网格: /Game/Robot/dji_spark/DJI_Spark_Fly
    - 动画: /Game/Robot/dji_spark/DJI_Spark_Fly_Anim
    
    Example:
        spark = DJISpark(client, agent)
        spark.takeoff(altitude=500)
        spark.fly_to(Location(1000, 2000, 500), speed=0.5)
        spark.hover()
    """

    def __init__(self, client, agent: "Agent"):
        self.client = client
        self.agent = agent
        self.name = agent.name
        self._default_speed = 0.5  # 默认飞行速度（建议不超过1）

    # ==================== 速度控制 ====================

    def set_velocity(self, vx: float, vy: float, vz: float) -> None:
        """
        设置飞行速度（世界坐标系）
        
        DJI Spark 朝向固定为 0，直接使用世界坐标系速度。

        Args:
            vx: 世界 X 方向速度
            vy: 世界 Y 方向速度
            vz: 垂直速度（正值向上）
        """
        # vyaw 固定为 0，不旋转
        cmd = f"vbp {self.name} set_move {vx} {vy} {vz} 0"
        self.client.client.request(cmd, -1)

    def hover(self) -> None:
        """悬停（停止所有移动）"""
        self.set_velocity(0, 0, 0)

    def stop(self) -> None:
        """停止移动（同 hover）"""
        self.hover()

    # ==================== 飞行控制 ====================

    def fly_to(self, target: Location, speed: float = None, threshold: float = 50) -> None:
        """
        飞行到目标位置（世界坐标系）

        Args:
            target: 目标位置（世界坐标）
            speed: 飞行速度，建议不超过1，None 使用默认速度
            threshold: 到达阈值
        """
        speed = speed or self._default_speed

        while True:
            current = self.agent.get_location()
            dx = target.x - current.x
            dy = target.y - current.y
            dz = target.z - current.z
            dist = current.distance_to(target)

            if dist < threshold:
                break

            # 计算世界坐标系速度向量（归一化后乘以速度）
            if dist > 0:
                ratio = speed / dist
                vx = dx * ratio
                vy = dy * ratio
                vz = dz * ratio
            else:
                vx, vy, vz = 0, 0, 0

            self.set_velocity(vx, vy, vz)
            time.sleep(0.05)

        self.hover()

    def fly_to_relative(self, dx: float, dy: float, dz: float, speed: float = None) -> None:
        """
        飞行到相对位置（世界坐标系偏移）

        Args:
            dx: 世界 X 方向偏移
            dy: 世界 Y 方向偏移
            dz: 世界 Z 方向偏移
            speed: 飞行速度
        """
        current = self.agent.get_location()
        target = Location(current.x + dx, current.y + dy, current.z + dz)
        self.fly_to(target, speed)

    # ==================== 起飞/降落 ====================

    def takeoff(self, altitude: float = 100, speed: float = 0.5) -> None:
        """
        起飞到相对当前位置的高度

        Args:
            altitude: 相对当前位置的目标高度
            speed: 上升速度，建议不超过1
        """
        target_z = self.agent.get_location().z + altitude
        self.set_velocity(0, 0, speed)

        while self.agent.get_location().z < target_z:
            time.sleep(0.05)

        self.hover()

    def land(self, speed: float = 0.3, min_altitude: float = 50) -> None:
        """
        降落

        Args:
            speed: 下降速度
            min_altitude: 最低高度阈值
        """
        self.set_velocity(0, 0, -speed)
        prev_z = self.agent.get_location().z

        while True:
            time.sleep(0.05)
            current_z = self.agent.get_location().z
            if current_z <= min_altitude or current_z >= prev_z:
                break
            prev_z = current_z

        self.hover()

    # ==================== 高度控制 ====================

    def ascend(self, dz: float, speed: float = 0.5) -> None:
        """上升指定高度"""
        target_z = self.agent.get_location().z + dz
        self.set_velocity(0, 0, speed)

        while self.agent.get_location().z < target_z:
            time.sleep(0.05)

        self.hover()

    def descend(self, dz: float, speed: float = 0.5) -> None:
        """下降指定高度"""
        target_z = self.agent.get_location().z - dz
        self.set_velocity(0, 0, -speed)

        while self.agent.get_location().z > target_z:
            time.sleep(0.05)

        self.hover()

    # ==================== 动画控制 ====================

    def play_animation(self, anim_name: str = "DJI_Spark_Fly_Anim") -> None:
        """
        播放动画
        
        Args:
            anim_name: 动画名称，默认为飞行动画
        """
        cmd = f"vbp {self.name} play_anim {anim_name}"
        self.client.client.request(cmd, -1)

    def stop_animation(self) -> None:
        """停止动画"""
        cmd = f"vbp {self.name} stop_anim"
        self.client.client.request(cmd, -1)

    # ==================== 状态查询 ====================

    def get_location(self) -> Location:
        """获取当前位置"""
        return self.agent.get_location()

    def get_rotation(self) -> Rotation:
        """获取当前旋转（朝向固定为 0）"""
        return self.agent.get_rotation()

    def get_altitude(self) -> float:
        """获取当前高度"""
        return self.agent.get_location().z

    def get_speed(self) -> float:
        """获取当前速度"""
        cmd = f"vbp {self.name} get_speed"
        res = self.client.client.request(cmd)
        try:
            return float(res)
        except (ValueError, TypeError):
            return 0.0

    # ==================== 物理设置 ====================

    def set_physics(self, enabled: bool) -> None:
        """设置物理模拟"""
        cmd = f"vbp {self.name} set_phy {1 if enabled else 0}"
        self.client.client.request(cmd, -1)

    def set_gravity(self, enabled: bool) -> None:
        """设置重力"""
        cmd = f"vbp {self.name} set_gravity {1 if enabled else 0}"
        self.client.client.request(cmd, -1)

    def reset(self) -> None:
        """重置无人机"""
        self.client.client.request(f"vbp {self.name} reset")

    def __repr__(self):
        return f"DJISpark(agent='{self.name}')"
