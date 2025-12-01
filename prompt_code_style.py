"""
UnrealZoo 代码风格规范

0. 架构设计 (参考 CARLA 风格)
============================
本项目参考 CARLA 的 API 设计风格，主要体现在：

- Transform: 封装 Location, Rotation, Scale
- Agent: 封装 UE 中的智能体，提供 get/set 方法
- AgentManager: 类似 CARLA 的 World，负责 spawn_agent

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
