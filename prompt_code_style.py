"""
UnrealZoo 代码风格规范

0. 架构设计
==========

0.1 图论管理 (NetworkX)
-----------------------
本项目使用 NetworkX 图论来统一管理多智能体及其关系：

核心组件:
- EntityGraph: 基于 NetworkX DiGraph 的实体图管理器
- EntityType: 实体类型枚举 (DRONE, CHARACTER, ANIMAL, OBJECT, LANDMARK, VEHICLE)
- RelationType: 关系类型枚举 (NEAR, TRACKING, VISIBLE, OWNS, COLLISION, FOLLOW)
- AgentManager: 持有 EntityGraph，提供高层 API

设计思路:
- 节点 (Node): 每个 Agent 实体是图中的一个节点
- 边 (Edge): 实体间的关系是有向边，带有关系类型和属性
- 类型索引: 内部维护 _type_index 加速按类型查询

这种设计的好处:
1. 统一管理异构实体：人/动物/无人机/物品都是节点，只是类型不同
2. 关系建模自然：追踪、跟随、碰撞、视野内等关系都是边
3. 空间查询高效：邻居查询、路径规划、区域检测都是图操作
4. 可视化友好：支持多种布局的图可视化

示例:
```python
from agent.agent_manager import AgentManager
from agent.entity_graph import EntityType, RelationType

# 初始化
manager = AgentManager(client, config)
manager.spawn_from_config()

# 按类型查询
drones = manager.get_by_type(EntityType.DRONE)
characters = manager.get_by_type(EntityType.CHARACTER)

# 建立关系
manager.add_relation("drone_0", "player_1", RelationType.TRACKING)
manager.add_relation("drone_0", "drone_1", RelationType.NEAR, distance=100)

# 查询关系
targets = manager.get_targets("drone_0", RelationType.TRACKING)  # drone_0 追踪谁
trackers = manager.get_sources("player_1", RelationType.TRACKING)  # 谁在追踪 player_1
neighbors = manager.get_neighbors("drone_0", RelationType.NEAR)  # drone_0 附近有谁

# 更新空间关系 (每帧调用)
manager.update_spatial_relations(distance_threshold=1000)

# 图算法
path = manager.graph.shortest_path("drone_0", "player_1")
components = manager.graph.get_connected_components()

# 可视化
manager.graph.visualize(save_path="graph.png")
manager.graph.visualize_multi_view(save_path="graph_multi.png")
manager.graph.visualize_spatial(save_path="graph_spatial.png")

# 导出图数据 (可传给 Unreal 或其他系统)
graph_data = manager.export_graph()
```

0.2 CARLA 风格 API
------------------
本项目参考 CARLA 的 API 设计风格：

- Transform: 封装 Location, Rotation, Scale
- Agent: 封装 UE 中的智能体，提供 get/set 方法
- Sensor: 封装传感器（Camera, Depth 等），可绑定到 Agent
- AgentManager: 类似 CARLA 的 World，负责 spawn 和管理

核心概念 - Agent 统一抽象:
在本项目中，Agent 是一个广义概念，代表场景中任何可交互的实体：
- Robot/Drone: 可移动的机器人或无人机
- Camera: 固定或移动的摄像头
- Sensor: 各类传感器（深度、激光雷达等）
- Vehicle: 车辆
- Character: 角色

命名约定:
- 使用 Agent（智能体）而非 Actor
- Agent 强调"有自主决策能力的实体"，更适合多智能体仿真场景

示例:
```python
from agent.agent_manager import AgentManager
from agent.transform import Transform, Location, Rotation, Scale

# 手动 spawn
transform = Transform(
    location=Location(x=1000, y=2000, z=100),
    rotation=Rotation(pitch=0, yaw=0, roll=0),
    scale=Scale(x=0.1, y=0.1, z=0.1)
)
drone = manager.spawn("BP_drone01_C", "drone_1", transform)

# Agent 操作
drone.set_location(Location(x=1500, y=2500, z=200))
drone.get_location()
drone.destroy()

# 从 JSON 配置批量 spawn
agents = manager.spawn_from_config()

# Sensor 绑定
camera = manager.spawn_sensor("sensor.camera.rgb", attach_to=drone)
depth = manager.spawn_sensor("sensor.camera.depth", attach_to=drone)

# Sensor 操作
image = camera.get_image()

# 通过 Agent 访问 Sensor
drone.get_sensors()
drone.get_sensor("camera.rgb")
drone.has_sensor()

# 字典式访问
agent = manager["drone_0"]
if "drone_0" in manager:
    ...
for agent in manager:
    ...
```


1. Import 顺序 (PEP 8)
======================
# Standard library imports
import sys
from pathlib import Path

# Third-party library imports
import numpy as np
from unrealcv.launcher import RunUnreal

# Local project imports
from config.config_manager import config_manager


2. 代码格式化
============
使用 Black: `black xx.py`


3. 配置管理
==========
不使用 argparse，统一用 config_manager 读取 YAML 配置。

```python
from config.config_manager import config_manager

# 读取参数
ue_binary = config_manager.get_ue_binary_path()
resolution = config_manager.get("rendering.resolution", [640, 480])

# 加载 Agent 配置
env_config = config_manager.load_env_config()
```

配置文件: config/config_parameter.yaml
```yaml
ue_binary_path: "/home/ubuntu/UnrealEnv/..."  # UE Binary 绝对路径

env:
  map: "Grass_Hills"              # UE 地图名
  agent_config: "multi_drone.json" # Agent 配置文件

rendering:
  resolution: [640, 480]
```

Agent 配置文件: config/agent_config/{agent_config}
"""
