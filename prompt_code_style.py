"""
UnrealZoo 代码风格规范

0. 架构设计 (参考 CARLA 风格)
============================
本项目参考 CARLA 的 API 设计风格，主要体现在：

- Transform: 封装 Location, Rotation, Scale
- Agent: 封装 UE 中的智能体，提供 get/set 方法
- Sensor: 封装传感器（Camera, Depth 等），可绑定到 Agent
- AgentManager: 类似 CARLA 的 World，负责 spawn_agent 和 spawn_sensor

核心概念 - Agent 统一抽象:
-------------------------
在本项目中，Agent 是一个广义概念，代表场景中任何可交互的实体：
- Robot/Drone: 可移动的机器人或无人机
- Camera: 固定或移动的摄像头
- Sensor: 各类传感器（深度、激光雷达等）
- Vehicle: 车辆
- Character: 角色

这种设计的好处：
1. 统一的 API：所有实体都有 get_location/set_location/destroy 等方法
2. 灵活的组合：Sensor 可以 attach_to 任何 Agent
3. 易于扩展：新增实体类型只需继承 Agent 基类

命名约定:
- 使用 Agent（智能体）而非 Actor
- Agent 强调"有自主决策能力的实体"，更适合多智能体仿真场景

示例:
```python
from agent import AgentManager, Transform, Location, Rotation, Scale

# 手动 spawn
transform = Transform(
    location=Location(x=1000, y=2000, z=100),
    rotation=Rotation(pitch=0, yaw=0, roll=0),
    scale=Scale(x=0.1, y=0.1, z=0.1)
)
drone = agent_manager.spawn_agent("BP_drone01_C", "drone_1", transform)

# Agent 操作
drone.set_location(Location(x=1500, y=2500, z=200))
drone.get_location()
drone.destroy()

# 从 JSON 配置批量 spawn
agents = agent_manager.spawn_agents_from_config()

# Sensor 绑定（CARLA 风格）
camera = agent_manager.spawn_sensor("sensor.camera.rgb", attach_to=drone)
depth = agent_manager.spawn_sensor("sensor.camera.depth", attach_to=drone)

# Sensor 操作
image = camera.get_image()
camera.listen(lambda img: process(img))  # 注册回调
camera.tick()  # 手动触发采集

# 通过 Agent 访问 Sensor
drone.get_sensors()  # 获取所有绑定的传感器
drone.get_sensor("camera.rgb")  # 获取指定类型的传感器
drone.has_sensor()  # 检查是否有传感器
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
