"""
Agent Manager - 管理智能体的创建和控制
"""
# Standard library imports
from typing import Any, Dict, List, Optional

# Third-party library imports
import numpy as np

# Local imports
from agent.agent import Agent
from agent.transform import Location, Rotation, Scale, Transform


class AgentManager:
    """
    Agent 管理器，负责 spawn 和管理 agents
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
        self.agents: Dict[str, Agent] = {}
        self.safe_starts = env_config.get("safe_start", [])

    def spawn_agent(
        self,
        class_name: str,
        name: str,
        transform: Transform,
        enable_physics: bool = False,
    ) -> Agent:
        """
        Spawn 单个 agent

        Args:
            class_name: 蓝图类名 (如 "BP_drone01_C")
            name: 对象名称 (如 "drone_1")
            transform: 变换信息 (位置、旋转、缩放)
            enable_physics: 是否启用物理

        Returns:
            Agent 对象
        """
        loc = transform.location
        rot = transform.rotation
        scale = transform.scale

        # 构建 spawn 命令
        cmds = [
            f"vset /objects/spawn {class_name} {name}",
            f"vset /object/{name}/location {loc.x} {loc.y} {loc.z}",
            f"vset /object/{name}/rotation {rot.pitch} {rot.yaw} {rot.roll}",
            f"vset /object/{name}/scale {scale.x} {scale.y} {scale.z}",
            f"vbp {name} set_phy {1 if enable_physics else 0}",
        ]

        # 执行命令
        for cmd in cmds:
            self.client.client.request(cmd)

        # 创建 Agent 对象
        agent = Agent(self.client, name, class_name, transform)
        self.agents[name] = agent

        print(f"   ✅ Spawned: {name} ({class_name}) at {loc}")
        return agent

    def spawn_agents_from_config(self) -> List[Agent]:
        """
        从 JSON 配置中 spawn 所有 agents

        Returns:
            已创建的 Agent 列表
        """
        agents_config = self.env_config.get("agents", {})
        spawned_agents = []

        for agent_type, config in agents_config.items():
            names = config.get("name", [])
            class_names = config.get("class_name", [])
            scale_list = config.get("scale", [1, 1, 1])

            for i, name in enumerate(names):
                class_name = class_names[i] if i < len(class_names) else class_names[0]

                # 判断是否需要物理
                enable_physics = class_name not in ["bp_character_C", "target_C"]

                # 获取出生点
                spawn_loc = self._get_spawn_location(i)

                # 构建 Transform
                transform = Transform(
                    location=Location.from_list(spawn_loc),
                    rotation=Rotation(),
                    scale=Scale.from_list(scale_list),
                )

                # Spawn agent
                agent = self.spawn_agent(
                    class_name=class_name,
                    name=name,
                    transform=transform,
                    enable_physics=enable_physics,
                )
                spawned_agents.append(agent)

        return spawned_agents

    def _get_spawn_location(self, index: int) -> List[float]:
        """获取出生点位置"""
        if index < len(self.safe_starts):
            return self.safe_starts[index]
        elif self.safe_starts:
            return self.safe_starts[np.random.randint(len(self.safe_starts))]
        else:
            return [0, 0, 100]

    def get_agent(self, name: str) -> Optional[Agent]:
        """获取 Agent"""
        return self.agents.get(name)

    def get_agents(self) -> List[Agent]:
        """获取所有 Agent"""
        return list(self.agents.values())

    def destroy_agent(self, name: str) -> bool:
        """销毁指定 Agent"""
        agent = self.agents.get(name)
        if agent:
            agent.destroy()
            del self.agents[name]
            return True
        return False

    def destroy_all(self) -> None:
        """销毁所有 Agent"""
        for name in list(self.agents.keys()):
            self.destroy_agent(name)
