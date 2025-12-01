"""
Python 代码风格规范指南

本项目遵循 PEP 8 的 import 分组规范，将导入语句分为三个部分，
每个部分之间用空行分隔。

Import 顺序规范:
================

1. Standard library imports (标准库导入)
   - Python 内置模块
   - 例如: sys, os, json, argparse, pathlib, typing 等

2. Third-party library imports (第三方库导入)
   - 通过 pip 安装的外部包
   - 例如: numpy, gym, unrealcv, torch 等

3. Local project imports (本地项目导入)
   - 项目内部的模块
   - 例如: config.config_manager, gym_unrealcv.envs 等

示例:
=====

```python
# Standard library imports
import sys
import json
from pathlib import Path
from typing import Dict, List, Optional

# Third-party library imports
import numpy as np
import gym
from unrealcv.launcher import RunUnreal
from unrealcv.api import UnrealCv_API

# Local project imports
from config.config_manager import config_manager
from gym_unrealcv.envs.base_env import BaseEnv
```

注意事项:
========
- 每个分组内部按字母顺序排列
- 分组之间用一个空行分隔
- 使用注释标明每个分组的类型
- 优先使用 `from x import y` 而非 `import x` (当只需要特定功能时)
- 避免使用 `from x import *`


代码格式化:
==========
使用 Black 格式化工具统一代码风格（空格、换行等）

使用方法:
```bash
black xx.py
```

Black 会自动处理:
- 统一缩进为 4 个空格
- 行长度限制 (默认 88 字符)
- 运算符周围的空格
- 逗号后的空格
- 函数参数的换行
- 字符串引号统一为双引号


配置管理规范:
============
不要使用 argparse 命令行参数，统一使用 YAML 配置文件管理参数。

❌ 不推荐:
```python
import argparse
parser = argparse.ArgumentParser()
parser.add_argument('--config', type=str, default='xxx.json')
parser.add_argument('--resolution', type=int, nargs=2, default=[640, 480])
args = parser.parse_args()
```

✅ 推荐:
```python
from config.config_manager import config_manager

# 从 config/config_parameter.yaml 读取参数
env_config = config_manager.get("env_config", "Track/Grass_Hills.json")
resolution = config_manager.get("rendering.resolution", [640, 480])
```

优点:
- 配置集中管理，便于维护
- 支持嵌套配置 (点状访问如 "rendering.resolution")
- 配置文件可版本控制，便于追踪变更
- 避免命令行参数过长
"""
