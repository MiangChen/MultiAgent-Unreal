"""
Agent Manager - 管理智能体的创建和控制
"""
# Standard library imports
from typing import Any, Dict, List, Optional, Type

# Third-party library imports
import numpy as np

# Local imports
from agent.agent import Agent
from agent.sensor import SensorRgb, SensorDepth, Sensor
from agent.transform import Location, Rotation, Scale, Transform


# Sensor 类型注册表（类似 CARLA 的 blueprint library）
SENSOR_BLUEPRINTS: Dict[str, Type[Sensor]] = {
    "sensor.camera.rgb": SensorRgb,
    "sensor.camera.depth": SensorDepth,
}


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
        cam_id: int = -1,
        enable_physics: bool = False,
        auto_detect_camera: bool = True,
    ) -> Agent:
        """
        Spawn 单个 agent

        Args:
            class_name: 蓝图类名 (如 "BP_drone01_C")
            name: 对象名称 (如 "drone_1")
            transform: 变换信息 (位置、旋转、缩放)
            cam_id: 绑定的摄像头 ID，-1 表示自动检测
            enable_physics: 是否启用物理
            auto_detect_camera: 是否自动检测新增的 camera

        Returns:
            Agent 对象
        """
        loc = transform.location
        rot = transform.rotation
        scale = transform.scale

        # 使用 unrealcv API spawn 对象（会自动注册新 camera）
        self.client.set_new_obj(class_name, name)

        # 设置位置、旋转、缩放
        self.client.set_obj_location(name, [loc.x, loc.y, loc.z])
        self.client.set_obj_rotation(name, [rot.pitch, rot.yaw, rot.roll])
        self.client.set_obj_scale(name, [scale.x, scale.y, scale.z])

        # 设置物理
        cmd = f"vbp {name} set_phy {1 if enable_physics else 0}"
        self.client.client.request(cmd)

        # 自动检测新增的 camera（set_new_obj 已经注册了 camera）
        if auto_detect_camera and cam_id == -1:
            cam_num = self.client.get_camera_num()
            if cam_num > 0:
                # 使用最后一个 camera
                cam_id = cam_num - 1

        # 创建 Agent 对象
        agent = Agent(self.client, name, class_name, transform, cam_id)
        self.agents[name] = agent

        print(f"   ✅ Spawned: {name} ({class_name}) at {loc}, cam_id={cam_id}")
        return agent

    def spawn_agents_from_config(self, auto_detect_camera: bool = True) -> List[Agent]:
        """
        从 JSON 配置中 spawn 所有 agents

        Args:
            auto_detect_camera: 是否自动检测 camera（推荐 True）

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

                # Spawn agent（cam_id=-1 表示自动检测）
                agent = self.spawn_agent(
                    class_name=class_name,
                    name=name,
                    transform=transform,
                    cam_id=-1,
                    enable_physics=enable_physics,
                    auto_detect_camera=auto_detect_camera,
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

    # ==================== Static Agent 生成 ====================

    def spawn_static(
        self,
        name: str,
        transform: Transform,
        shape: str = "cube",
        color: tuple = None,
    ) -> Agent:
        """
        生成静态对象（Cube、Sphere 等）

        Args:
            name: 对象名称
            transform: 变换信息
            shape: 形状类型 ("cube", "sphere", "cylinder", "cone")
            color: RGB 颜色 (0-255)，如 (255, 0, 0) 为红色

        Returns:
            Agent 对象

        Example:
            cube = agent_manager.spawn_static(
                "obstacle_1",
                Transform(location=Location(100, 200, 50), scale=Scale(2, 2, 2)),
                shape="cube",
                color=(255, 0, 0)
            )
        """
        # 使用 UE 项目中可用的蓝图类
        shape_blueprints = {
            "cube": "BP_GrabbableObject_C",
            "sphere": "BP_GrabbableObject_C",
            "cylinder": "BP_GrabbableObject_C",
            "object": "BP_GrabbableObject_C",
        }
        class_name = shape_blueprints.get(shape.lower(), "BP_GrabbableObject_C")
        loc = transform.location
        rot = transform.rotation
        scale = transform.scale

        # 直接发送命令（绕过 set_new_obj 的 numpy bug）
        cmd = f"vset /objects/spawn {class_name} {name}"
        res = self.client.client.request(cmd)
        if "error" in str(res).lower():
            print(f"   ⚠️ Failed to spawn {name}: {res}")
            return None

        # 设置位置、旋转、缩放
        self.client.set_obj_location(name, [loc.x, loc.y, loc.z])
        self.client.set_obj_rotation(name, [rot.pitch, rot.yaw, rot.roll])
        self.client.set_obj_scale(name, [scale.x, scale.y, scale.z])

        # 设置颜色
        if color:
            r, g, b = color
            self.client.set_obj_color(name, [r, g, b])

        # 创建 Agent 对象
        agent = Agent(self.client, name, f"static.{shape}", transform, cam_id=-1)
        self.agents[name] = agent

        print(f"   🧊 Spawned static: {name} ({shape}) at {loc}")
        return agent

    def destroy_all(self) -> None:
        """销毁所有 Agent"""
        for name in list(self.agents.keys()):
            self.destroy_agent(name)

    # ==================== CARLA 风格的 Sensor 管理 ====================

    def spawn_sensor(
        self,
        blueprint_id: str,
        attach_to: Agent,
        sensor_id: Optional[int] = None,
    ) -> Sensor:
        """
        Spawn 传感器并绑定到 Agent（CARLA 风格）

        Args:
            blueprint_id: 传感器蓝图 ID (如 "sensor.camera.rgb")
            attach_to: 要绑定的 Agent
            sensor_id: 传感器 ID，None 则使用 Agent 的 cam_id

        Returns:
            Sensor 对象

        Example:
            # CARLA 风格用法
            camera = manager.spawn_sensor("sensor.camera.rgb", attach_to=drone)
            camera.listen(lambda img: process(img))
        """
        # 获取 sensor 类
        sensor_class = SENSOR_BLUEPRINTS.get(blueprint_id)
        if sensor_class is None:
            raise ValueError(f"Unknown sensor blueprint: {blueprint_id}")

        # 确定 sensor_id
        sid = sensor_id if sensor_id is not None else attach_to.cam_id
        if sid < 0:
            raise ValueError(f"Agent {attach_to.name} has no valid cam_id")

        # 检查 camera ID 是否在有效范围内
        cam_num = self.client.get_camera_num()
        if sid >= cam_num:
            print(f"   ⚠️ cam_id {sid} 超出范围 (max={cam_num-1})，跳过")
            return None

        # 创建并绑定 sensor
        sensor = sensor_class(self.client, sid)
        sensor.attach_to(attach_to)

        print(f"   📷 Sensor spawned: {blueprint_id} -> {attach_to.name} (id={sid})")
        return sensor

    def get_blueprint_library(self) -> List[str]:
        """获取可用的传感器蓝图列表（CARLA 风格）"""
        return list(SENSOR_BLUEPRINTS.keys())

    # ==================== 场景清理 ====================

    def cleanup_scene(self, patterns: List[str] = None) -> int:
        """
        清理场景中已存在的对象（在 spawn 新 agents 之前调用）

        Args:
            patterns: 要删除的对象名称前缀列表，默认删除所有 BP_ 开头的对象

        Returns:
            删除的对象数量

        Example:
            # 删除所有 BP_ 开头的对象
            agent_manager.cleanup_scene()

            # 只删除特定类型
            agent_manager.cleanup_scene(patterns=["BP_Drone", "BP_Character"])
        """
        if patterns is None:
            patterns = ["BP_"]

        # 获取场景中所有对象
        objects = self.client.get_objects()
        destroyed_count = 0

        for obj_name in objects:
            # 检查是否匹配任一 pattern
            if any(obj_name.startswith(p) for p in patterns):
                cmd = f"vset /object/{obj_name}/destroy"
                self.client.client.request(cmd)
                print(f"   🗑️ Cleaned: {obj_name}")
                destroyed_count += 1

        print(f"   ✅ Scene cleanup: {destroyed_count} objects removed")
        return destroyed_count

    # ==================== 发现场景中已存在的对象 ====================

    def discover_agents(
        self,
        patterns: List[str] = None,
        class_name: str = "static",
    ) -> List[Agent]:
        """
        发现场景中已存在的对象并纳入管理（Static Agents）

        Args:
            patterns: 对象名称匹配模式列表，如 ["Cube", "SM_"]
            class_name: 分配给这些对象的类名标识

        Returns:
            发现的 Agent 列表

        Example:
            # 发现所有 Cube 对象
            cubes = agent_manager.discover_agents(patterns=["Cube"])

            # 发现所有静态网格
            statics = agent_manager.discover_agents(patterns=["SM_", "StaticMesh"])

            # 之后可以像普通 agent 一样操作
            for cube in cubes:
                cube.get_location()
                cube.set_location(Location(x=100, y=200, z=50))
        """
        if patterns is None:
            patterns = ["Cube", "SM_"]

        objects = self.client.get_objects()
        discovered = []

        for obj_name in objects:
            # 跳过已管理的对象
            if obj_name in self.agents:
                continue

            # 检查是否匹配
            if any(p in obj_name for p in patterns):
                # 获取对象当前位置
                try:
                    loc = self.client.get_obj_location(obj_name)
                    rot = self.client.get_obj_rotation(obj_name)
                    transform = Transform(
                        location=Location.from_list(loc),
                        rotation=Rotation.from_list(rot),
                        scale=Scale(),
                    )
                except Exception:
                    transform = Transform()

                # 创建 Agent 对象（static agent 没有 camera）
                agent = Agent(
                    self.client,
                    name=obj_name,
                    class_name=class_name,
                    transform=transform,
                    cam_id=-1,
                )
                self.agents[obj_name] = agent
                discovered.append(agent)
                print(f"   🔍 Discovered: {obj_name}")

        print(f"   ✅ Discovered {len(discovered)} static agents")
        return discovered
