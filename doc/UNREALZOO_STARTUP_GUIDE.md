# UnrealZoo 机器人配置与启动指南

## 概述

UnrealZoo 是一个基于 Unreal Engine 5.5 + UnrealCV 的多机器人仿真框架。

## 1. 启动流程

### 1.1 整体架构

```
Python 应用层 (gym.make → base_env.py → Character_API)
        ↓
UnrealCV 通信层 (RunUnreal → TCP Client → UnrealCV API)
        ↓
Unreal Engine 5.5 (UnrealCV Plugin + Blueprint Actors + NavMesh)
```

### 1.2 启动代码

```python
import gym
import gym_unrealcv
from gym_unrealcv.envs.wrappers import configUE

# 1. 创建环境
env = gym.make('UnrealTrack-Old_Town-ContinuousColor-v0')

# 2. 配置环境
env = configUE.ConfigUEWrapper(env, offscreen=False, resolution=(320, 240))

# 3. 设置智能体类型
env.unwrapped.agents_category = ['player', 'animal']

# 4. 重置环境（触发 UE 启动）
obs = env.reset()

# 5. 主循环
while True:
    actions = [[0, 50], [0, 100]]  # [人类动作, 机器狗动作]
    obs, rewards, done, info = env.step(actions)
    if done:
        break

env.close()
```

## 2. 配置文件

### 2.1 位置

```
gym_unrealcv/envs/setting/Track/
├── Old_Town.json           # 基础配置
├── Old_Town_with_dog.json  # 包含机器狗
```

### 2.2 格式示例

```json
{
    "env_name": "Old_Town",
    "env_bin": "UnrealZoo_UE5_5_Linux_v1.0.5/Linux/UnrealZoo_UE5_5/Binaries/Linux/UnrealZoo_UE5_5",
    "env_map": "Old_Town",
    
    "agents": {
        "player": {
            "name": ["BP_Character_C_1"],
            "cam_id": [1],
            "class_name": ["bp_character_C"],
            "internal_nav": true,
            "move_action_continuous": {"high": [30, 100], "low": [-30, -100]}
        },
        "animal": {
            "name": ["BP_animal_C_1"],
            "cam_id": [2],
            "class_name": ["BP_animal_C"],
            "internal_nav": true,
            "move_action_continuous": {"high": [30, 150], "low": [-30, -150]}
        }
    },
    
    "safe_start": [[1045.767, 19755.0, 26802.335]],
    "reset_area": [-696.498, 2525.655, 18221.0, 20926.388, 26755.586, 27420.0]
}
```

## 3. 机器人类型

| 类名 | 类型 | 说明 |
|------|------|------|
| BP_Character_C | player | 人类角色 |
| BP_animal_C | animal | 动物/机器狗 |
| BP_drone01_C | drone | 无人机 |
| BP_BaseCar_C | car | 车辆 |

### 机器狗外观 ID

- 0: Dog (Default)
- 1-2: Dog Variants
- 3: Wolf
- 4: Fox
- 5: Cat
- 6-10: 马、斑马、驴、鹿、牛
- 11-16: 大象、犀牛、河马、熊、狮子、老虎

## 4. 核心代码流程

### 4.1 base_env.py 初始化

```python
class UnrealCv_base(gym.Env):
    def __init__(self, setting_file, ...):
        # 加载配置
        setting = misc.load_env_setting(setting_file)
        self.agent_configs = setting['agents']
        self.agents = misc.convert_dict(self.agent_configs)
        
        # 初始化智能体列表
        self.player_list = list(self.agents.keys())
        
        # 创建 UE 启动器
        self.ue_binary = RunUnreal(ENV_BIN=env_bin, ENV_MAP=env_map)
```

### 4.2 reset() 流程

```python
def reset(self):
    if not self.launched:
        self.launched = self.launch_ue_env()
        self.init_agents()
    
    init_poses = self.sample_init_pose(...)
    for i, obj in enumerate(self.player_list):
        self.unrealcv.set_obj_location(obj, init_poses[i])
    
    return observations
```

### 4.3 launch_ue_env()

```python
def launch_ue_env(self):
    env_ip, env_port = self.ue_binary.start(
        resolution=self.resolution,
        offscreen=self.offscreen_rendering,
        sleep_time=10
    )
    
    self.unrealcv = Character_API(port=env_port, ip=env_ip, ...)
    self.unrealcv.set_map(self.env_name)
    return True
```

## 5. UnrealCV API

```python
# 位置控制
unrealcv.set_obj_location(obj, [x, y, z])
unrealcv.get_obj_location(obj)

# 移动控制
unrealcv.set_move_bp(obj, [angular_vel, linear_vel])

# 导航
unrealcv.nav_to_goal(obj, [x, y, z])

# 外观
unrealcv.set_appearance(obj, appearance_id)

# 速度
unrealcv.set_max_speed(obj, speed)

# 图像
unrealcv.get_image(cam_id, mode='lit')
```

## 6. 机器狗示例

```python
import gym
import gym_unrealcv
from gym_unrealcv.envs.wrappers import configUE

env = gym.make('UnrealTrack-Old_Town-ContinuousColor-v0')
env.unwrapped.config_file = 'gym_unrealcv/envs/setting/Track/Old_Town_with_dog.json'
env.unwrapped.agents_category = ['player', 'animal']

env = configUE.ConfigUEWrapper(env, offscreen=False, resolution=(320, 240))
obs = env.reset()

# 设置机器狗外观
dog_name = env.unwrapped.player_list[1]
env.unwrapped.unrealcv.set_appearance(dog_name, 0)

while True:
    actions = [[0, 50], [0, 100]]
    obs, rewards, done, info = env.step(actions)
    if done:
        break

env.close()
```

## 7. 环境变量

```bash
export UnrealEnv=~/UnrealEnv
```
