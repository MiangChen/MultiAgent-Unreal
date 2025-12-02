"""
Drone 类 - 封装 UE 中的无人机操作（BP_drone）

无人机不支持 NavMesh 导航，所有移动使用速度控制
速度是相对于无人机本地坐标系，需要根据 yaw 角进行坐标变换
"""
import math
import time
from typing import TYPE_CHECKING

from agent.transform import Location, Rotation

if TYPE_CHECKING:
    from agent.agent import Agent


class Drone:
    """
    Drone 类，封装无人机特有的操作（飞行、悬停、速度控制等）
    
    注意：set_velocity 的 vx, vy 是相对于无人机本地坐标系
    - vx: 无人机前方（正值向前）
    - vy: 无人机右方（正值向右）
    - vz: 垂直方向（正值向上）
    
    fly_to 等方法会自动进行世界坐标到本地坐标的转换
    
    Example:
        drone = Drone(client, agent)
        drone.takeoff(altitude=1000)
        drone.fly_to(Location(1000, 2000, 500), speed=0.5)
        drone.hover()
    """

    def __init__(self, client, agent: "Agent"):
        self.client = client
        self.agent = agent
        self.name = agent.name
        self._default_speed = 0.5  # 默认飞行速度（建议不超过1）

    # ==================== 坐标变换 ====================

    def _world_to_local(self, world_vx: float, world_vy: float) -> tuple:
        """
        将世界坐标系的速度转换为无人机本地坐标系
        
        Args:
            world_vx: 世界坐标系 X 方向速度
            world_vy: 世界坐标系 Y 方向速度
            
        Returns:
            (local_vx, local_vy): 本地坐标系速度
        """
        yaw = self.agent.get_rotation().yaw
        yaw_rad = math.radians(yaw)
        
        # 旋转变换：世界坐标 -> 本地坐标
        cos_yaw = math.cos(yaw_rad)
        sin_yaw = math.sin(yaw_rad)
        
        local_vx = world_vx * cos_yaw + world_vy * sin_yaw
        local_vy = -world_vx * sin_yaw + world_vy * cos_yaw
        
        return local_vx, local_vy

    # ==================== 速度控制 ====================

    def set_velocity(self, vx: float, vy: float, vz: float, vyaw: float = 0) -> None:
        """
        设置飞行速度（本地坐标系）

        Args:
            vx: 前方速度（正值向前）
            vy: 右方速度（正值向右）
            vz: 垂直速度（正值向上）
            vyaw: 偏航角速度
        """
        cmd = f"vbp {self.name} set_move {vx} {vy} {vz} {vyaw}"
        self.client.client.request(cmd, -1)

    def set_velocity_world(self, world_vx: float, world_vy: float, vz: float, vyaw: float = 0) -> None:
        """
        设置飞行速度（世界坐标系）
        
        Args:
            world_vx: 世界 X 方向速度
            world_vy: 世界 Y 方向速度
            vz: 垂直速度（正值向上）
            vyaw: 偏航角速度
        """
        local_vx, local_vy = self._world_to_local(world_vx, world_vy)
        self.set_velocity(local_vx, local_vy, vz, vyaw)

    def hover(self) -> None:
        """悬停（停止所有移动）"""
        self.set_velocity(0, 0, 0, 0)

    def stop(self) -> None:
        """停止移动（同 hover）"""
        self.hover()

    # ==================== 飞行控制 ====================

    def fly_to(self, target: Location, speed: float = None, threshold: float = 50) -> None:
        """
        飞行到目标位置（使用速度控制，世界坐标系）

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
                world_vx = dx * ratio
                world_vy = dy * ratio
                vz = dz * ratio
            else:
                world_vx, world_vy, vz = 0, 0, 0

            # 转换为本地坐标系并设置速度
            self.set_velocity_world(world_vx, world_vy, vz, 0)
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
        self.set_velocity(0, 0, speed, 0)

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
        self.set_velocity(0, 0, -speed, 0)
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
        self.set_velocity(0, 0, speed, 0)

        while self.agent.get_location().z < target_z:
            time.sleep(0.05)

        self.hover()

    def descend(self, dz: float, speed: float = 0.5) -> None:
        """下降指定高度"""
        target_z = self.agent.get_location().z - dz
        self.set_velocity(0, 0, -speed, 0)

        while self.agent.get_location().z > target_z:
            time.sleep(0.05)

        self.hover()

    # ==================== 旋转控制 ====================

    def rotate(self, delta_yaw: float, speed: float = 0.3) -> None:
        """
        旋转指定角度

        Args:
            delta_yaw: 旋转角度（正值顺时针）
            speed: 旋转速度
        """
        target_yaw = self.agent.get_rotation().yaw + delta_yaw
        direction = 1 if delta_yaw > 0 else -1
        self.set_velocity(0, 0, 0, speed * direction)

        while True:
            current_yaw = self.agent.get_rotation().yaw
            if direction > 0 and current_yaw >= target_yaw:
                break
            if direction < 0 and current_yaw <= target_yaw:
                break
            time.sleep(0.05)

        self.hover()

    # ==================== 状态查询 ====================

    def get_location(self) -> Location:
        """获取当前位置"""
        return self.agent.get_location()

    def get_rotation(self) -> Rotation:
        """获取当前旋转"""
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
        return f"Drone(agent='{self.name}')"
