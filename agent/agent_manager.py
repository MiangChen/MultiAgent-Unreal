"""
Agent Manager - 基于图论的多智能体管理
"""
from typing import Any, Dict, List, Optional, Type

from agent.agent import Agent
from agent.entity_graph import EntityGraph, EntityType, RelationType
from agent.sensor import Sensor, SensorDepth, SensorRgb
from agent.transform import Location, Rotation, Scale, Transform


SENSOR_BLUEPRINTS: Dict[str, Type[Sensor]] = {
    "sensor.camera.rgb": SensorRgb,
    "sensor.camera.depth": SensorDepth,
}

# 类名到实体类型映射
CLASS_TYPE_MAP = {
    "BP_drone01_C": EntityType.DRONE,
    "BP_Character_C": EntityType.CHARACTER,
    "bp_character_C": EntityType.CHARACTER,
    "target_C": EntityType.CHARACTER,
    "BP_Animal_C": EntityType.ANIMAL,
}

# 配置字符串到实体类型映射
CONFIG_TYPE_MAP = {
    "drone": EntityType.DRONE,
    "character": EntityType.CHARACTER,
    "player": EntityType.CHARACTER,
    "animal": EntityType.ANIMAL,
    "object": EntityType.OBJECT,
    "vehicle": EntityType.VEHICLE,
    "landmark": EntityType.LANDMARK,
}

# 不启用物理的类
NO_PHYSICS_CLASSES = {"bp_character_C", "target_C", "BP_Character_C"}


class AgentManager:
    """基于图论的 Agent 管理器"""

    def __init__(self, client, env_config: Dict[str, Any]):
        self.client = client
        self.env_config = env_config
        self.graph = EntityGraph()
        self.safe_starts = env_config.get("safe_start", [])

    # ==================== 访问 ====================

    def get(self, name: str) -> Optional[Agent]:
        return self.graph.get_entity(name)

    def get_all(self) -> List[Agent]:
        return self.graph.get_all_entities()

    def get_by_type(self, entity_type: EntityType) -> List[Agent]:
        return self.graph.get_entities_by_type(entity_type)

    def __getitem__(self, name: str) -> Agent:
        agent = self.graph.get_entity(name)
        if agent is None:
            raise KeyError(f"Entity '{name}' not found")
        return agent

    def __contains__(self, name: str) -> bool:
        return name in self.graph

    def __len__(self) -> int:
        return len(self.graph)

    def __iter__(self):
        return iter(self.graph.get_all_entities())

    # ==================== 关系 ====================

    def add_relation(self, source: str, target: str, relation: RelationType, **attrs) -> bool:
        return self.graph.add_relation(source, target, relation, **attrs)

    def remove_relation(self, source: str, target: str, relation: RelationType = None) -> bool:
        return self.graph.remove_relation(source, target, relation)

    def get_targets(self, source: str, relation: RelationType = None) -> List[str]:
        return self.graph.get_targets(source, relation)

    def get_sources(self, target: str, relation: RelationType = None) -> List[str]:
        return self.graph.get_sources(target, relation)

    def get_neighbors(self, name: str, relation: RelationType = None) -> List[str]:
        return self.graph.get_neighbors(name, relation)

    def update_spatial_relations(self, distance_threshold: float = 500.0) -> int:
        return self.graph.update_spatial_relations(distance_threshold)

    def export_graph(self) -> Dict[str, Any]:
        return self.graph.to_dict()

    # ==================== Spawn ====================

    def spawn(
        self,
        class_name: str,
        name: str,
        transform: Transform,
        entity_type: EntityType = None,
        enable_physics: bool = False,
        auto_detect_camera: bool = True,
        **properties
    ) -> Agent:
        """Spawn 单个实体"""
        loc, rot, scale = transform.location, transform.rotation, transform.scale

        self.client.set_new_obj(class_name, name)
        self.client.set_obj_location(name, [loc.x, loc.y, loc.z])
        self.client.set_obj_rotation(name, [rot.pitch, rot.yaw, rot.roll])
        self.client.set_obj_scale(name, [scale.x, scale.y, scale.z])
        self.client.client.request(f"vbp {name} set_phy {1 if enable_physics else 0}")

        cam_id = -1
        if auto_detect_camera:
            cam_num = self.client.get_camera_num()
            cam_id = cam_num - 1 if cam_num > 0 else -1

        agent = Agent(self.client, name, class_name, transform, cam_id)
        etype = entity_type or CLASS_TYPE_MAP.get(class_name, EntityType.OBJECT)
        self.graph.add_entity(agent, etype, **properties)

        print(f"   ✅ Spawned: {name} ({etype.value}), cam_id={cam_id}")
        return agent

    def spawn_from_config(self, auto_detect_camera: bool = True) -> List[Agent]:
        """从配置 spawn 所有实体"""
        spawned = []
        for type_str, agent_list in self.env_config.get("agents", {}).items():
            etype = CONFIG_TYPE_MAP.get(type_str.lower(), EntityType.OBJECT)

            for cfg in agent_list:
                name = cfg.get("name", f"{type_str}_{cfg.get('id', 0)}")
                class_name = cfg.get("class_name", "StaticMeshActor")
                transform = Transform(
                    location=Location.from_list(cfg.get("position", [0, 0, 100])),
                    rotation=Rotation.from_list(cfg.get("rotation", [0, 0, 0])),
                    scale=Scale.from_list(cfg.get("scale", [1, 1, 1])),
                )
                enable_physics = not cfg.get("disable_gravity", False) and class_name not in NO_PHYSICS_CLASSES

                extra = {k: v for k, v in cfg.items()
                         if k not in {'id', 'name', 'class_name', 'position', 'rotation', 'scale', 'disable_gravity'}}

                spawned.append(self.spawn(class_name, name, transform, etype, enable_physics, auto_detect_camera, **extra))

        print(f"   📊 Graph: {self.graph}")
        return spawned

    def spawn_static(self, name: str, transform: Transform, shape: str = "cube", color: tuple = None, **props) -> Optional[Agent]:
        """生成静态物体"""
        loc, rot, scale = transform.location, transform.rotation, transform.scale
        res = self.client.client.request(f"vset /objects/spawn BP_GrabbableObject_C {name}")
        if "error" in str(res).lower():
            print(f"   ⚠️ Failed: {name}: {res}")
            return None

        self.client.set_obj_location(name, [loc.x, loc.y, loc.z])
        self.client.set_obj_rotation(name, [rot.pitch, rot.yaw, rot.roll])
        self.client.set_obj_scale(name, [scale.x, scale.y, scale.z])
        if color:
            self.client.set_obj_color(name, list(color))

        agent = Agent(self.client, name, f"static.{shape}", transform, cam_id=-1)
        self.graph.add_entity(agent, EntityType.OBJECT, shape=shape, **props)
        print(f"   🧊 Static: {name} at {loc}")
        return agent

    # ==================== 销毁 ====================

    def destroy(self, name: str) -> bool:
        agent = self.graph.get_entity(name)
        if agent:
            agent.destroy()
            self.graph.remove_entity(name)
            return True
        return False

    def destroy_all(self):
        for name in list(self.graph.graph.nodes):
            self.destroy(name)

    # ==================== Sensor ====================

    def spawn_sensor(self, blueprint_id: str, attach_to: Agent, sensor_id: int = None) -> Optional[Sensor]:
        sensor_class = SENSOR_BLUEPRINTS.get(blueprint_id)
        if not sensor_class:
            raise ValueError(f"Unknown sensor: {blueprint_id}")

        sid = sensor_id if sensor_id is not None else attach_to.cam_id
        if sid < 0:
            raise ValueError(f"Agent {attach_to.name} has no cam_id")

        cam_num = self.client.get_camera_num()
        if sid >= cam_num:
            print(f"   ⚠️ cam_id {sid} out of range (max={cam_num - 1})")
            return None

        sensor = sensor_class(self.client, sid)
        sensor.attach_to(attach_to)
        print(f"   📷 Sensor: {blueprint_id} -> {attach_to.name}")
        return sensor

    # ==================== 场景 ====================

    def cleanup_scene(self, patterns: List[str] = None) -> int:
        patterns = patterns or ["BP_"]
        count = 0
        for obj in self.client.get_objects():
            if any(obj.startswith(p) for p in patterns):
                self.client.client.request(f"vset /object/{obj}/destroy")
                print(f"   🗑️ Cleaned: {obj}")
                count += 1
        print(f"   ✅ Cleanup: {count} removed")
        return count

    def discover(self, patterns: List[str] = None, entity_type: EntityType = EntityType.OBJECT) -> List[Agent]:
        patterns = patterns or ["Cube", "SM_"]
        discovered = []
        for obj in self.client.get_objects():
            if obj in self.graph or not any(p in obj for p in patterns):
                continue
            try:
                transform = Transform(
                    location=Location.from_list(self.client.get_obj_location(obj)),
                    rotation=Rotation.from_list(self.client.get_obj_rotation(obj)),
                )
            except Exception:
                transform = Transform()

            agent = Agent(self.client, obj, "discovered", transform, cam_id=-1)
            self.graph.add_entity(agent, entity_type)
            discovered.append(agent)
            print(f"   🔍 Discovered: {obj}")

        print(f"   ✅ Discovered {len(discovered)}, graph: {self.graph}")
        return discovered
