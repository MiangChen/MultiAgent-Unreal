"""
Agent Manager - 统一管理机器人的创建和控制
"""
# Standard library imports
from typing import Any, Dict, List, Optional

# Third-party library imports
import numpy as np


class AgentManager:
    """
    Agent 管理器，负责从 JSON 配置中加载并 spawn 机器人
    """

    def __init__(self, client, env_config: Dict[str, Any]):
        """
        初始化 Agent 管理器

        Args:
            client: UnrealCv_API 客户端
            env_config: 环境配置 (从 JSON 加载)
        """
        self.client = client
        self.env_config = env_config
        self.agents: Dict[str, Dict] = {}  # 已创建的 agent 信息
        self.safe_starts = env_config.get("safe_start", [])

    def spawn_agents_from_config(self, agent_types: Optional[List[str]] = None) -> Dict[str, List[str]]:
        """
        从配置中 spawn 所有或指定类型的 agents

        Args:
            agent_types: 要 spawn 的 agent 类型列表，如 ["player", "drone"]
                        如果为 None，则 spawn 所有配置的 agents

        Returns:
            已创建的 agent 名称字典 {agent_type: [agent_names]}
        """
        agents_config = self.env_config.get("agents", {})
        spawned = {}

        for agent_type, config in agents_config.items():
            if agent_types and agent_type not in agent_types:
                continue

            spawned[agent_type] = []
            names = config.get("name", [])
            class_names = config.get("class_name", [])

            for i, name in enumerate(names):
                class_name = class_names[i] if i < len(class_names) else class_names[0]

                # 获取出生点
                spawn_loc = self._get_spawn_location(i)

                # Spawn agent
                self.spawn_agent(
                    class_name=class_name,
                    obj_name=name,
                    location=spawn_loc,
                    agent_type=agent_type,
                    config=config,
                )
                spawned[agent_type].append(name)

        return spawned

    def spawn_agent(
        self,
        class_name: str,
        obj_name: str,
        location: List[float],
        rotation: List[float] = None,
        agent_type: str = "unknown",
        config: Dict = None,
    ) -> str:
        """
        Spawn 单个 agent

        Args:
            class_name: UE 蓝图类名 (如 "bp_character_C")
            obj_name: 对象名称 (如 "BP_Character_C_1")
            location: 出生位置 [x, y, z]
            rotation: 旋转角度 [pitch, yaw, roll]，默认 [0, 0, 0]
            agent_type: agent 类型 (player/drone/animal 等)
            config: agent 配置

        Returns:
            创建的对象名称
        """
        if rotation is None:
            rotation = [0, 0, 0]

        x, y, z = location
        pitch, yaw, roll = rotation

        # 根据 agent 类型决定是否启用物理
        enable_physics = 0 if class_name in ["bp_character_C", "target_C"] else 1

        # 获取 scale（默认 [1, 1, 1]）
        scale = config.get("scale", [1, 1, 1]) if config else [1, 1, 1]
        sx, sy, sz = scale

        # 构建 spawn 命令
        cmds = [
            f"vset /objects/spawn {class_name} {obj_name}",
            f"vset /object/{obj_name}/location {x} {y} {z}",
            f"vset /object/{obj_name}/rotation {pitch} {yaw} {roll}",
            f"vset /object/{obj_name}/scale {sx} {sy} {sz}",
            f"vbp {obj_name} set_phy {enable_physics}",
        ]

        # 执行命令
        for cmd in cmds:
            self.client.client.request(cmd)

        # 记录 agent 信息
        self.agents[obj_name] = {
            "class_name": class_name,
            "agent_type": agent_type,
            "location": location,
            "rotation": rotation,
            "config": config or {},
        }

        print(f"   ✅ Spawned {agent_type}: {obj_name} at {location}")
        return obj_name

    def _get_spawn_location(self, index: int) -> List[float]:
        """获取出生点位置"""
        if index < len(self.safe_starts):
            return self.safe_starts[index]
        elif self.safe_starts:
            # 随机选择一个出生点
            return self.safe_starts[np.random.randint(len(self.safe_starts))]
        else:
            return [0, 0, 100]  # 默认位置

    def get_agent(self, name: str) -> Optional[Dict]:
        """获取 agent 信息"""
        return self.agents.get(name)

    def get_agents_by_type(self, agent_type: str) -> List[str]:
        """获取指定类型的所有 agent 名称"""
        return [
            name for name, info in self.agents.items() if info["agent_type"] == agent_type
        ]

    def destroy_agent(self, name: str) -> bool:
        """销毁 agent"""
        if name not in self.agents:
            return False

        cmd = f"vset /object/{name}/destroy"
        self.client.client.request(cmd)
        del self.agents[name]
        print(f"   🗑️ Destroyed: {name}")
        return True

    def destroy_all(self):
        """销毁所有 agent"""
        for name in list(self.agents.keys()):
            self.destroy_agent(name)
