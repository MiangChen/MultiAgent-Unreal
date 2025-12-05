UnrealZoo 代码风格规范

重要：不要使用英文的复数，保持单数。

前置阅读
========
在开始编码前，请先阅读 UE5 API 变更文档：doc/UE5_API_CHANGES.md

0. 架构设计
==========

0.1 CARLA 风格架构
------------------
本项目采用 CARLA 风格架构：

核心特点:
- 参数存储在实体属性上（而非外部配置文件）
- 行为由实体自身驱动
- 支持运行时动态修改

CARLA 风格设计原则:
1. 属性在实体上 - Robot 有 ScanRadius, FollowTarget 等属性，类似 CARLA Vehicle 的 max_speed
2. 运行时可修改 - 可以随时 Robot->SetFollowTarget(Target)
3. 行为由实体驱动 - 实体自身属性决定行为

示例:
```cpp
// CARLA 风格：参数在实体上，运行时可修改
Robot->ScanRadius = 300.f;
Robot->SetFollowTarget(HumanCharacter);

// StateTree Task 从实体获取参数，而非自己存储
AMACharacter* Target = Robot->GetFollowTarget();
float Distance = Robot->ScanRadius;
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


4. Unreal Engine C++ 命名规范
============================

4.1 UE 类名前缀规则 (强制)
-------------------------
UE 的 UHT (Unreal Header Tool) 强制要求特定前缀，否则编译报错：

- A 前缀: 继承自 AActor 的类 (AMAAgent, AMAGameMode)
- U 前缀: 继承自 UObject 的类 (UMyComponent)
- F 前缀: 普通结构体 (FVector, FTransform)
- E 前缀: 枚举类型 (EAgentType)
- I 前缀: 接口类 (IMyInterface)
- T 前缀: 模板类 (TArray, TMap)

4.2 本项目命名约定
-----------------
核心原则: 文件名与类名保持一致，统一使用 MA 前缀

为什么用 MA 前缀:
1. 避免与 UE 引擎头文件冲突 (GameMode.h, Character.h 等已被 UE 占用)
2. MA = MultiAgent，表明是本项目的类
3. 文件名和类名一致，便于查找和维护

文件结构 (模块化):
```
unreal_project/Source/MultiAgent/
├── AgentManager/                    # AgentManager 模块 (司命)
│   ├── MAAgentSubsystem.h/cpp       # UMAAgentSubsystem - Agent 管理子系统
│   ├── MAAgent.h/cpp                # AMAAgent - Agent 基类
│   ├── MAHumanAgent.h/cpp           # AMAHumanAgent - 人类 Agent
│   ├── MARobotDogAgent.h/cpp        # AMARobotDogAgent - 机器狗 Agent
│   └── MACharacter.h/cpp            # AMACharacter - 带摄像机的角色
├── Core/                            # 核心框架
│   ├── MAGameMode.h/cpp             # AMAGameMode - 游戏模式
│   └── MAPlayerController.h/cpp     # AMAPlayerController - 玩家控制器
├── RelationManager/                 # RelationManager 模块 (司缘) - 待开发
├── MapManager/                      # MapManager 模块 (司图) - 待开发
└── MultiAgent.h/cpp                 # 模块定义
```

类继承关系:
```
UWorldSubsystem (UE)
    └── UMAAgentSubsystem (AgentManager 司命)

ACharacter (UE)
    └── AMAAgent (Agent 基类)
            ├── AMAHumanAgent (人类)
            └── AMARobotDogAgent (机器狗)
    └── AMACharacter (带摄像机的角色)

AGameModeBase (UE)
    └── AMAGameMode

APlayerController (UE)
    └── AMAPlayerController
```

当前类一览表:

AgentManager 模块:

| 类名                | 文件位置                          | 简介                                      |
|--------------------|----------------------------------|------------------------------------------|
| UMAAgentSubsystem  | AgentManager/MAAgentSubsystem.h  | Agent 管理子系统，负责生命周期管理           |
| AMAAgent           | AgentManager/MAAgent.h           | Agent 基类，提供移动、ID、类型等基础功能      |
| AMAHumanAgent      | AgentManager/MAHumanAgent.h      | 人类 Agent，使用 Manny 模型和动画蓝图       |
| AMARobotDogAgent   | AgentManager/MARobotDogAgent.h   | 机器狗 Agent，带行走/待机动画切换           |
| AMACharacter       | AgentManager/MACharacter.h       | 带俯视摄像机的角色，用于玩家控制             |
| EAgentType         | AgentManager/MAAgent.h           | Agent 类型枚举 (Human, RobotDog, Drone)   |

Core 模块:

| 类名                | 文件位置                          | 简介                                      |
|--------------------|----------------------------------|------------------------------------------|
| AMAGameMode        | Core/MAGameMode.h                | 游戏模式，协调各子系统，初始化场景           |
| AMAPlayerController| Core/MAPlayerController.h        | 玩家控制器，处理鼠标点击移动逻辑            |

4.3 UMAAgentSubsystem 使用示例
-----------------------------
```cpp
// 获取 AgentSubsystem
UMAAgentSubsystem* AgentSubsystem = GetWorld()->GetSubsystem<UMAAgentSubsystem>();

// 生成 Agent
AMAAgent* Dog = AgentSubsystem->SpawnAgent(
    AMARobotDogAgent::StaticClass(),
    FVector(0, 0, 100),
    FRotator::ZeroRotator,
    -1,  // 自动分配 ID
    EAgentType::RobotDog
);

// 查询 Agent
TArray<AMAAgent*> AllAgents = AgentSubsystem->GetAllAgents();
TArray<AMAAgent*> Dogs = AgentSubsystem->GetAgentsByType(EAgentType::RobotDog);
AMAAgent* Agent = AgentSubsystem->GetAgentByID(0);

// 批量操作
AgentSubsystem->MoveAllAgentsTo(FVector(1000, 1000, 0), 200.f);
AgentSubsystem->StopAllAgents();

// 监听事件
AgentSubsystem->OnAgentSpawned.AddDynamic(this, &AMyClass::HandleAgentSpawned);
```

4.4 generated.h 规则
-------------------
UHT 生成的头文件名必须与源文件名一致：

```cpp
// MAAgent.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MAAgent.generated.h"  // 必须与文件名一致

UCLASS()
class MULTIAGENT_API AMAAgent : public ACharacter
{
    GENERATED_BODY()
    // ...
};
```

4.5 禁止使用的文件名
------------------
以下文件名会与 UE 引擎冲突，禁止使用：
- GameMode.h (冲突: Engine/.../GameFramework/GameMode.h)
- PlayerController.h (冲突)
- Character.h (冲突)
- Actor.h (冲突)
- Pawn.h (冲突)
- Agent.h (可能冲突，建议用 MAAgent.h)

4.6 新增类的命名模板
------------------
```cpp
// MANewClass.h
#pragma once

#include "CoreMinimal.h"
#include "MAAgent.h"  // 或其他父类
#include "MANewClass.generated.h"

UCLASS()
class MULTIAGENT_API AMANewClass : public AMAAgent
{
    GENERATED_BODY()

public:
    AMANewClass();
};
```

```cpp
// MANewClass.cpp
#include "MANewClass.h"

AMANewClass::AMANewClass()
{
    // 构造函数
}
```


5. UE 项目编译指南
=================

5.1 环境要求
-----------
- Unreal Engine 5.5 源码版（从 GitHub 克隆编译）
- Linux: clang 18+ (Ubuntu 22.04+)
- UE 源码路径: /home/ubuntu/PycharmProjects/UnrealEngine


5.2 编译命令
-----------
```bash
# 使用 UE 的 Build.sh 脚本编译 Editor 版本（推荐）
/home/ubuntu/PycharmProjects/UnrealEngine/Engine/Build/BatchFiles/Linux/Build.sh \
    MultiAgentEditor Linux Development \
    -Project="/home/ubuntu/PycharmProjects/MultiAgent-Unreal/unreal_project/MultiAgent.uproject" \
    -WaitMutex 2>&1 | tail -60

# 或者直接用 UE 编辑器打开 .uproject 文件，会自动编译
```

5.3 清理重新编译
---------------
如果遇到编译问题，删除缓存后重新编译：
```bash
cd unreal_project
rm -rf Binaries Intermediate DerivedDataCache

# 重新编译
/home/ubuntu/PycharmProjects/UnrealEngine/Engine/Build/BatchFiles/Linux/Build.sh \
    MultiAgentEditor Linux Development \
    -Project="/home/ubuntu/PycharmProjects/MultiAgent-Unreal/unreal_project/MultiAgent.uproject" \
    -WaitMutex
```

5.4 不需要提交到 Git 的文件夹
---------------------------
以下文件夹由编译/运行时自动生成，已在 .gitignore 中排除：
- `Binaries/` - 编译产物
- `Intermediate/` - 中间编译文件
- `DerivedDataCache/` - 资产缓存
- `Saved/` - 日志、配置、自动保存


6. 测试按键
==========

运行游戏后可用以下按键测试：

| 按键 | 功能 |
|-----|------|
| **T** | 在玩家前方生成一个机器狗 |
| **Y** | 打印当前所有 Agent 信息（数量、类型、位置、是否持有物品） |
| **U** | 销毁最后一个 Agent |
| **I** | 在鼠标位置生成可拾取方块 |
| **P** | 所有 Human Agent 尝试拾取附近物品 |
| **O** | 所有 Human Agent 放下持有的物品 |
| **G** | 所有 RobotDog 开始巡逻场景中的 PatrolPath |
| **Tab** | 切换摄像机视角 |
| **0** | 返回观察者视角 |
| 左键 | 移动所有 Human Agent 到点击位置 |
| 右键 | 移动所有 RobotDog 到点击位置 |

注意：所有 Agent 统一由 `UMAAgentSubsystem` 管理，拾取/放下功能通过 GAS 实现。


7. GAS 使用示例
==============

```cpp
// Agent 拾取物品
AMAAgent* Agent = ...;
Agent->TryPickup();  // 自动检测附近物品并拾取

// Agent 放下物品
Agent->TryDrop();

// 检查是否持有物品
if (Agent->IsHoldingItem())
{
    AMAPickupItem* Item = Agent->GetHeldItem();
}
```


8. StateTree Task 设计规范
=========================

8.1 核心原则
-----------
StateTree Task 不应该有太多可配置参数，参数应该存储在 Agent (如 Robot) 身上，Task 从 Agent 获取。

好处：
- 参数集中管理，便于调整
- 同一参数可被多个 Task/Ability 复用
- 运行时可动态修改

8.2 参数存储位置
---------------
```cpp
// MARobotDogCharacter.h - 参数存储在 Robot 上
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coverage")
float ScanRadius = 200.f;  // 扫描半径，用于 Coverage 和 Follow

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
TWeakObjectPtr<AMACharacter> FollowTarget;  // 跟随目标
```

8.3 Task 设计示例
----------------
```cpp
// MASTTask_Follow.h - Task 不存储参数，只有注释说明
USTRUCT(meta = (DisplayName = "MA Follow"))
struct MULTIAGENT_API FMASTTask_Follow : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

    // 注意: 跟随目标从 Robot 的 FollowTarget 获取
    // 注意: 跟随距离从 Robot 的 ScanRadius 获取

protected:
    // ...
};
```

```cpp
// MASTTask_Follow.cpp - 从 Robot 获取参数
EStateTreeRunStatus FMASTTask_Follow::EnterState(...) const
{
    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Owner);
    
    // 从 Robot 获取跟随目标
    AMACharacter* Target = Robot->GetFollowTarget();
    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] No FollowTarget set"));
        return EStateTreeRunStatus::Failed;
    }
    
    // GA_Follow 会从 Robot 获取 ScanRadius 作为跟随距离
    ASC->TryActivateFollow(Target);
    // ...
}
```

8.4 Ability 获取参数示例
-----------------------
```cpp
// GA_Follow.cpp - Ability 从 Robot 获取参数
void UGA_Follow::ActivateAbility(...)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 从 RobotDog 获取 ScanRadius 作为跟随距离
    if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Character))
    {
        FollowDistance = Robot->ScanRadius;
    }
    // ...
}
```

8.5 使用方式
-----------
```cpp
// 设置参数后发送命令
Robot->SetFollowTarget(HumanCharacter);  // 设置目标
Robot->ScanRadius = 300.f;               // 可选：调整距离

// 发送 Follow 命令，StateTree 会自动使用 Robot 上的参数
ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("Command.Follow"));
```
