"""
UnrealZoo 代码风格规范

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

# 加载环境配置 (自动拼接 {task}/{map}.json)
env_config = config_manager.load_env_config()
```

配置文件: config/config_parameter.yaml
```yaml
ue_binary_path: "/home/ubuntu/UnrealEnv/..."  # UE Binary 绝对路径

env:
  task: "Track"           # 对应 setting 下的文件夹
  map: "Grass_Hills"      # 对应 JSON 文件名
  action_type: "Continuous"
  observation_type: "Color"

rendering:
  resolution: [640, 480]
```
"""
