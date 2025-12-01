"""
Agent Manager - CARLA 风格的 Agent 管理器
"""
# Standard library imports
from typing import Any, Dict, List, Optional

# Third-party library imports
import numpy as np

# Local imports
from agent.actor import Actor
from agent.transform import Location, Rotation, Scale, Transform


class AgentManager:
    """
    Agent 管理器，负责 spawn 和管理 actors
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
        self.actors: Dict[str, Actor] = {}
        self.safe_starts = env_config.get("safe_start", [])

    def spawn_actor(
        self,
        class_name: str,
        name: str,
        transform: Transform,
        enable_physics: bool = False,
    ) -> Actor:
        """
        Spawn 单个 actor (CARLA 风格)

        Args:
            class_name: 蓝图类名 (如 "BP_drone01_C")
            name: 对象名称 (如 "drone_1")
            transform: 变换信息 (位置、旋转、缩放)
            enable_physics: 是否启用物理

        Returns:
            Actor 对象
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

        # 创建 Actor 对象
        actor = Actor(self.client, name, class_name, transform)
        self.actors[name] = actor

        print(f"   ✅ Spawned: {name} ({class_name}) at {loc}")
        return actor

    def spawn_agents_from_config(self) -> List[Actor]:
        """
        从 JSON 配置中 spawn 所有 agents

        Returns:
            已创建的 Actor 列表
        """
        agents_config = self.env_config.get("agents", {})
        spawned_actors = []

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

                # Spawn actor
                actor = self.spawn_actor(
                    class_name=class_name,
                    name=name,
                    transform=transform,
                    enable_physics=enable_physics,
                )
                spawned_actors.append(actor)

        return spawned_actors

    def _get_spawn_location(self, index: int) -> List[float]:
        """获取出生点位置"""
        if index < len(self.safe_starts):
            return self.safe_starts[index]
        elif self.safe_starts:
            return self.safe_starts[np.random.randint(len(self.safe_starts))]
        else:
            return [0, 0, 100]

    def get_actor(self, name: str) -> Optional[Actor]:
        """获取 Actor"""
        return self.actors.get(name)

    def get_actors(self) -> List[Actor]:
        """获取所有 Actor"""
        return list(self.actors.values())

    def destroy_actor(self, name: str) -> bool:
        """销毁指定 Actor"""
        actor = self.actors.get(name)
        if actor:
            actor.destroy()
            del self.actors[name]
            return True
        return False

    def destroy_all(self) -> None:
        """销毁所有 Actor"""
        for name in list(self.actors.keys()):
            self.destroy_actor(name)
