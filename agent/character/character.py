"""
Character 类 - 封装 UE 中的角色操作（BP_Character）
"""
from typing import TYPE_CHECKING, List, Optional

if TYPE_CHECKING:
    from agent.agent import Agent


class Character:
    """
    Character 类，封装角色特有的操作（移动、动作、交互等）
    可以绑定到 Agent 上使用
    """

    def __init__(self, client, agent: "Agent"):
        """
        初始化 Character

        Args:
            client: UnrealCv_API 客户端
            agent: 绑定的 Agent 对象
        """
        self.client = client
        self.agent = agent
        self.name = agent.name
        self._is_carrying = False

    # ==================== 移动控制 ====================

    def set_move(self, params: List[float]) -> None:
        """
        设置移动参数

        Args:
            params: 移动参数
                - 2 params: [v_angle, v_linear] 用于平面移动（人、车、动物）
                - 4 params: [v_x, v_y, v_z, v_yaw] 用于 3D 移动（无人机）
        """
        params_str = " ".join([str(p) for p in params])
        cmd = f"vbp {self.name} set_move {params_str}"
        self.client.client.request(cmd, -1)

    def set_max_speed(self, speed: float) -> None:
        """设置最大速度"""
        cmd = f"vbp {self.name} set_speed {speed}"
        self.client.client.request(cmd)

    def set_acceleration(self, acc: float) -> None:
        """设置加速度"""
        cmd = f"vbp {self.name} set_acc {acc}"
        self.client.client.request(cmd, -1)

    def get_speed(self) -> float:
        """获取当前速度"""
        cmd = f"vbp {self.name} get_speed"
        res = self.client.client.request(cmd)
        return float(res) if res else 0.0

    # ==================== 动作控制 ====================

    def jump(self) -> None:
        """跳跃"""
        cmd = f"vbp {self.name} set_jump"
        self.client.client.request(cmd, -1)

    def crouch(self) -> None:
        """蹲下"""
        cmd = f"vbp {self.name} set_crouch"
        self.client.client.request(cmd, -1)

    def stand_up(self) -> None:
        """站起"""
        cmd = f"vbp {self.name} set_standup"
        self.client.client.request(cmd, -1)

    def lie_down(self, front_back: int = 100, left_right: int = 100) -> None:
        """躺下"""
        cmd = f"vbp {self.name} set_liedown {front_back} {left_right}"
        self.client.client.request(cmd, -1)

    # ==================== 物体交互 ====================

    def carry(self) -> None:
        """拾取/搬运物体"""
        cmd = f"vbp {self.name} carry_body"
        self.client.client.request(cmd, -1)
        self._is_carrying = True

    def drop(self) -> None:
        """放下物体"""
        cmd = f"vbp {self.name} drop_body"
        self.client.client.request(cmd, -1)
        self._is_carrying = False

    def is_carrying(self) -> bool:
        """检查是否正在搬运物体"""
        cmd = f"vbp {self.name} is_carrying"
        res = self.client.client.request(cmd)
        return "1" in str(res)

    def is_picked(self) -> bool:
        """检查是否被拾取"""
        cmd = f"vbp {self.name} is_picked"
        res = self.client.client.request(cmd)
        return "1" in str(res)

    # ==================== 导航 ====================

    def nav_to(self, x: float, y: float, z: float) -> None:
        """导航到目标位置"""
        cmd = f"vbp {self.name} nav_to_goal {x} {y} {z}"
        self.client.client.request(cmd, -1)

    def nav_to_object(self, target_obj: str, distance: float = 200) -> None:
        """导航到目标对象"""
        cmd = f"vbp {self.name} nav_to_target {target_obj} {distance}"
        self.client.client.request(cmd, -1)

    def nav_random(self, radius: float, loop: bool = False) -> List[float]:
        """随机导航"""
        cmd = f"vbp {self.name} nav_random {radius} {1 if loop else 0}"
        res = self.client.client.request(cmd)
        # 解析返回的位置
        try:
            coords = [float(x) for x in res.split()]
            return coords
        except:
            return []

    def set_nav_speed(self, max_speed: float) -> None:
        """设置导航最大速度"""
        cmd = f"vbp {self.name} set_nav_speed {max_speed}"
        self.client.client.request(cmd, -1)

    # ==================== 车辆交互 ====================

    def enter_exit_car(self, player_index: int = 0) -> None:
        """进出车辆"""
        cmd = f"vbp {self.name} enter_exit_car {player_index}"
        self.client.client.request(cmd, -1)

    # ==================== 门交互 ====================

    def open_door(self) -> None:
        """开门"""
        cmd = f"vbp {self.name} set_open_door 1"
        self.client.client.request(cmd, -1)

    def close_door(self) -> None:
        """关门"""
        cmd = f"vbp {self.name} set_open_door 0"
        self.client.client.request(cmd, -1)

    # ==================== 其他 ====================

    def reset(self) -> None:
        """重置角色"""
        cmd = f"vbp {self.name} reset"
        self.client.client.request(cmd)

    def set_appearance(self, appearance_id: int) -> None:
        """设置外观"""
        cmd = f"vbp {self.name} set_app {appearance_id}"
        self.client.client.request(cmd, -1)

    def set_random_behavior(self, enabled: bool = True) -> None:
        """设置随机行为"""
        cmd = f"vbp {self.name} set_random {1 if enabled else 0}"
        self.client.client.request(cmd, -1)

    def __repr__(self):
        return f"Character(agent='{self.name}')"
