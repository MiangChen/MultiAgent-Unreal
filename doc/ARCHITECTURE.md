# MultiAgent-Unreal 架构设计

## 1. 系统概述

MultiAgent-Unreal 是一个基于 Unreal Engine 5 的多智能体仿真框架，用于机器人、无人机、角色等异构智能体的协同仿真。

## 2. 核心架构：CARLA 风格 + State Tree + GAS

### 2.1 CARLA 风格设计

本项目采用 **CARLA 风格架构**，参数存储在实体上而非外部配置：

```
┌─────────────────────────────────────────────────────────────┐
│                    CARLA 风格参数管理                        │
│                                                             │
│  Robot (实体)                    Task/Ability (行为)         │
│  ┌─────────────────┐            ┌─────────────────┐        │
│  │ ScanRadius=200  │◄──────────│ MA Follow       │        │
│  │ FollowTarget    │  获取参数  │ MA Coverage     │        │
│  │ CoverageArea    │◄──────────│ MA Patrol       │        │
│  │ PatrolPath      │            └─────────────────┘        │
│  └─────────────────┘                                       │
└─────────────────────────────────────────────────────────────┘
```

**核心原则：**
- 参数存储在 Robot 上（如 `ScanRadius`, `FollowTarget`）
- Task/Ability 从 Robot 获取参数，不自己存储
- 同一参数可被多个 Task/Ability 复用
- 运行时可动态修改

**Robot 共用参数：**

| 属性 | 类型 | 用途 |
|------|------|------|
| `ScanRadius` | float | Follow 距离、Coverage 间距、Patrol 到达判定 |
| `FollowTarget` | AMACharacter* | 跟随目标 |
| `CoverageArea` | AActor* | 覆盖区域 |
| `PatrolPath` | AMAPatrolPath* | 巡逻路径 |
| `ChargeRate` | float | 充电速率（每秒恢复百分比，默认 20%） |

**参数复用示例：**
```cpp
// ScanRadius 被多个 Task 复用
// - MASTTask_Follow: 作为跟随距离
// - MASTTask_Coverage: 作为路径间距  
// - MASTTask_Patrol: 作为到达判定半径
float Distance = Robot->ScanRadius;
```

### 2.2 State Tree + GAS 黄金搭档

本项目采用 **State Tree + GAS** 黄金搭档架构：

```
State Tree (大脑 - 状态决策)          GAS (手脚 - 技能执行)
┌─────────────────────────┐         ┌─────────────────────────┐
│  Root                   │         │  Abilities              │
│  ├── Exploration State  │◄───────►│  ├── GA_Pickup          │
│  ├── PhotoMode State    │  Tags   │  ├── GA_Drop            │
│  └── Interaction State  │◄───────►│  ├── GA_TakePhoto       │
│      ├── Pickup         │         │  └── GA_Navigate        │
│      └── Dialogue       │         │                         │
└─────────────────────────┘         └─────────────────────────┘
```

- **State Tree**: 高层策略决策，管理 Character 的主模式（探索/拍照/交互）
- **GAS**: 具体技能执行，处理动画、冷却、消耗等
- **Gameplay Tags**: 两者之间的通信桥梁

## 3. 当前文件结构

```
unreal_project/Source/MultiAgent/
├── Core/                            # 核心框架 ✅ 已实现
│   ├── MAActorSubsystem.h/cpp       # Actor 管理子系统 (UWorldSubsystem)
│   ├── MAGameMode.h/cpp             # 游戏模式
│   └── MAPlayerController.h/cpp     # 玩家控制器 (Enhanced Input)
├── Character/                       # ACharacter 派生类 ✅ 已实现
│   ├── MACharacter.h/cpp            # 角色基类 (继承 ACharacter, 集成 GAS)
│   ├── MAHumanCharacter.h/cpp       # 人类角色
│   └── MARobotDogCharacter.h/cpp    # 机器狗角色
├── Actor/                           # 通用 AActor 派生类 ✅ 已实现
│   ├── MASensor.h/cpp               # 传感器基类 (继承 AActor)
│   ├── MACameraSensor.h/cpp         # 摄像头传感器
│   ├── MAPickupItem.h/cpp           # 可拾取物品
│   ├── MAChargingStation.h/cpp      # 充电站
│   ├── MAPatrolPath.h/cpp           # 巡逻路径
│   └── MACoverageArea.h/cpp         # 覆盖区域
├── GAS/                             # GAS 模块 ✅ 已实现
│   ├── MAAbilitySystemComponent.h/cpp  # GAS 核心组件
│   ├── MAGameplayTags.h/cpp         # Gameplay Tags 定义
│   ├── MAGameplayAbilityBase.h/cpp  # Ability 基类
│   └── Ability/                     # 具体技能
│       ├── GA_Pickup.h/cpp          # 拾取技能
│       ├── GA_Drop.h/cpp            # 放下技能
│       ├── GA_Navigate.h/cpp        # 导航技能
│       ├── GA_Follow.h/cpp          # 追踪技能
│       ├── GA_TakePhoto.h/cpp       # 拍照技能
│       ├── GA_Search.h/cpp          # 搜索技能
│       ├── GA_Observe.h/cpp         # 观察技能
│       ├── GA_Report.h/cpp          # 报告技能
│       ├── GA_Charge.h/cpp          # 充电技能
│       ├── GA_Formation.h/cpp       # 编队技能
│       └── GA_Avoid.h/cpp           # 避障技能
├── Input/                           # 输入系统 ✅ 已实现
│   ├── MAInputActions.h/cpp         # Input Actions 单例 (运行时创建)
│   ├── MAInputConfig.h/cpp          # 输入配置数据资产
│   └── MAInputComponent.h/cpp       # 增强输入组件
├── StateTree/                       # State Tree 模块 ✅ 已实现
│   ├── MAStateTreeComponent.h/cpp   # State Tree 组件
│   ├── Task/                        # State Tree Task
│   │   ├── MASTTask_ActivateAbility.h/cpp  # 激活 Ability 的 Task
│   │   ├── MASTTask_Navigate.h/cpp  # 导航 Task
│   │   ├── MASTTask_Patrol.h/cpp    # 巡逻 Task
│   │   ├── MASTTask_Charge.h/cpp    # 充电 Task
│   │   ├── MASTTask_Coverage.h/cpp  # 区域覆盖 Task
│   │   └── MASTTask_Follow.h/cpp    # 跟随 Task
│   └── Condition/                   # State Tree Condition
│       ├── MASTCondition_HasCommand.h/cpp  # 检查命令 Tag
│       ├── MASTCondition_LowEnergy.h/cpp   # 检查低电量
│       ├── MASTCondition_FullEnergy.h/cpp  # 检查满电量
│       └── MASTCondition_HasPatrolPath.h/cpp # 检查巡逻路径
├── MultiAgent.Build.cs              # 模块构建配置
└── MultiAgent.h/cpp                 # 模块定义
```

## 4. 类继承关系

```
ACharacter (UE) + IAbilitySystemInterface
    └── AMACharacter (角色基类，集成 GAS)
            ├── AMAHumanCharacter (人类)
            └── AMARobotDogCharacter (机器狗)

AActor (UE)
    ├── AMASensor (传感器基类)
    │       └── AMACameraSensor (摄像头传感器)
    ├── AMAPickupItem (可拾取物品)
    ├── AMAChargingStation (充电站)
    └── AMAPatrolPath (巡逻路径)

UAbilitySystemComponent (UE GAS)
    └── UMAAbilitySystemComponent

UGameplayAbility (UE GAS)
    └── UMAGameplayAbilityBase
            ├── UGA_Pickup
            ├── UGA_Drop
            ├── UGA_Navigate
            ├── UGA_Follow
            ├── UGA_TakePhoto
            ├── UGA_Search
            ├── UGA_Observe
            ├── UGA_Report
            ├── UGA_Charge
            ├── UGA_Formation
            └── UGA_Avoid

UWorldSubsystem (UE)
    └── UMAActorSubsystem (Character/Sensor 管理)
```

## 5. Manager 子系统设计

### 5.1 全局 Manager vs Character 组件

```
┌─────────────────────────────────────────────────────────────┐
│                    全局 Manager 层                           │
│                  UWorldSubsystem                             │
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │ActorSubsys  │  │RelationMgr  │  │ MapManager  │         │
│  │ 管理所有    │  │ 管理关系    │  │ 管理地图    │         │
│  │ Character   │  │ 之间的关系  │  │ 导航感知    │         │
│  │ 和 Sensor   │  │             │  │             │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ 管理
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Character 组件层                          │
│                  每个 Character 独立拥有                     │
│                                                             │
│   Character #1          Character #2          Character #3  │
│  ┌───────────┐         ┌───────────┐         ┌───────────┐ │
│  │ StateTree │         │ StateTree │         │ StateTree │ │
│  │ 决策      │         │ 决策      │         │ 决策      │ │
│  ├───────────┤         ├───────────┤         ├───────────┤ │
│  │ ASC       │         │ ASC       │         │ ASC       │ │
│  │ 技能执行  │         │ 技能执行  │         │ 技能执行  │ │
│  └───────────┘         └───────────┘         └───────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 5.2 Manager 列表

| Manager | 层级 | 功能介绍 | UE/C++ 实现方案 | 状态 |
|---------|------|---------|----------------|------|
| **ActorSubsystem** | 全局 | Character/Sensor 生命周期管理 | `UMAActorSubsystem` | ✅ 已实现 |
| **RelationManager** | 全局 | 实体关系管理 | `UMARelationSubsystem` | ❌ 待开发 |
| **MapManager** | 全局 | 地图感知 | `UNavigationSystemV1` | ❌ 待开发 |
| **StateTree** | Character级 | 状态决策 | `UMAStateTreeComponent` | ✅ 已实现 |
| **ASC** | Character级 | 技能执行 | `UMAAbilitySystemComponent` | ✅ 已实现 |

## 6. Characters vs Actors 设计 (UE 标准规范)

### 6.1 设计理念

按照 UE 官方项目规范（如 Lyra、ActionRPG），将场景实体分为两类：

| 目录 | 基类 | 特点 | 示例 |
|------|------|------|------|
| **Characters/** | `ACharacter` | 可移动、有 AI、有技能 | Human, RobotDog, Drone |
| **Actors/** | `AActor` | 通用 Actor（传感器、道具等） | Sensor, PickupItem |

### 6.2 Sensor 附着机制

Sensor 可以附着到任意 Character 上：

```cpp
// 生成 Camera Sensor 并附着到 Human
AMACameraSensor* Camera = ActorSubsystem->SpawnSensor(
    AMACameraSensor::StaticClass(), Location, Rotation, -1);
Camera->AttachToCharacter(Human, FVector(-200, 0, 100), FRotator(-10, 0, 0));
```

### 6.3 第三人称视角

每个 Human/RobotDog 生成时会自动附着一个第三人称摄像头：

| Character | 摄像头位置 | 说明 |
|-----------|-----------|------|
| Human | `(-200, 0, 100)` Pitch -10° | 后方 2m，高 1m，向下看 |
| RobotDog | `(-150, 0, 80)` Pitch -15° | 后方 1.5m，高 0.8m |

## 7. GAS 模块详解

### 7.1 Gameplay Tags 定义

```cpp
// State Tags (State Tree 状态)
State.Exploration          // 探索模式
State.PhotoMode            // 拍照模式
State.Interaction          // 交互模式
State.Interaction.Pickup   // 拾取中

// Ability Tags (GAS 技能)
Ability.Pickup             // 拾取技能
Ability.Drop               // 放下技能
Ability.TakePhoto          // 拍照技能
Ability.Patrol             // 巡逻技能
Ability.Search             // 搜索技能
Ability.Observe            // 观察技能
Ability.Report             // 报告技能
Ability.Charge             // 充电技能
Ability.Formation          // 编队技能
Ability.Avoid              // 避障技能

// Event Tags (新增)
Event.Target.Found         // 发现目标
Event.Target.Lost          // 丢失目标
Event.Charge.Complete      // 充电完成
Event.Patrol.WaypointReached  // 到达巡逻点

// Status Tags (新增)
Status.Patrolling          // 正在巡逻
Status.Searching           // 正在搜索
Status.Observing           // 正在观察
Status.Charging            // 正在充电
Status.InFormation         // 编队中
Status.Avoiding            // 避障中
Status.LowEnergy           // 低电量
Ability.Navigate           // 导航技能

// Event Tags (ST <-> GAS 通信)
Event.Pickup.Start         // 开始拾取
Event.Pickup.End           // 拾取完成

// Status Tags (状态标记)
Status.CanPickup           // 可以拾取
Status.Holding             // 正在持有物品
Status.Moving              // 正在移动
```

### 7.2 Ability 列表

| Ability | 功能 | 激活条件 | 阻止条件 |
|---------|------|---------|---------|
| `GA_Pickup` | 拾取物品 | 附近有可拾取物品 | Status.Holding |
| `GA_Drop` | 放下物品 | Status.Holding | - |
| `GA_Navigate` | 导航移动 | 有 AIController | - |
| `GA_Follow` | 追踪目标 | 有目标 Character | - |
| `GA_TakePhoto` | 拍照 | 有附着的 CameraSensor | - |
| `GA_Search` | 搜索 Human | 有附着的 CameraSensor | - |
| `GA_Observe` | 观察目标 | - | - |
| `GA_Report` | 报告信息 | - | - |
| `GA_Charge` | 充电 | 在充电站范围内 | - |
| `GA_Formation` | 编队移动 | 有 Leader | 无电量 |
| `GA_Avoid` | 避障 | - | - |

### 7.3 使用示例

```cpp
// Character 拾取物品
AMACharacter* Character = ...;
Character->TryPickup();  // 自动检测附近物品并拾取

// Character 放下物品
Character->TryDrop();

// 检查是否持有物品
if (Character->IsHoldingItem())
{
    AMAPickupItem* Item = Character->GetHeldItem();
}

// 导航到目标位置
Character->TryNavigateTo(FVector(100, 200, 0));

// 追踪另一个 Character
Character->TryFollowActor(TargetCharacter, 300.f);

// ========== 新增 Robot Abilities ==========

// 巡逻 (RobotDog)
AMARobotDogCharacter* Robot = ...;
TArray<FVector> Waypoints = {FVector(0,0,0), FVector(500,0,0), FVector(500,500,0)};
Robot->TryPatrol(Waypoints);

// 搜索 Human (需要附着 CameraSensor)
Robot->TrySearch();  // 发现 Human 时输出 "Find Target"

// 充电 (需要在 ChargingStation 范围内)
Robot->TryCharge();  // 恢复电量到 100%

// 编队跟随
AMACharacter* Leader = ...;
Robot->GetAbilitySystemComponent()->TryActivateFormation(Leader, EFormationType::Wedge, 0);
```

## 8. 追踪功能 (Ground Truth Tracking)

### 8.1 概述

基于 Ground Truth 的追踪功能，让一个 Character 可以持续跟随另一个 Character。

```
┌─────────────────────────────────────────────────────────────┐
│                    追踪工作原理                              │
│                                                             │
│  每 0.5 秒:                                                 │
│    1. 获取目标位置 (Ground Truth)                           │
│       TargetLocation = TargetCharacter->GetActorLocation() │
│                                                             │
│    2. 计算跟随位置 (保持距离)                                │
│       FollowLocation = TargetLocation + Direction * Dist   │
│                                                             │
│    3. 导航过去                                              │
│       AIController->MoveToLocation(FollowLocation)         │
└─────────────────────────────────────────────────────────────┘
```

### 8.2 GA_Follow 技能

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `FollowDistance` | 200.f | 与目标保持的距离 |
| `UpdateInterval` | 0.5f | 位置更新频率（秒） |
| `Repath_Threshold` | 100.f | 目标移动超过此距离才重新导航 |

### 8.3 使用方式

```cpp
// 获取两个 Character
AMACharacter* Hunter = ActorSubsystem->GetCharacterByID(0);
AMACharacter* Target = ActorSubsystem->GetCharacterByID(1);

// Hunter 开始追踪 Target，保持 300 单位距离
Hunter->TryFollowActor(Target, 300.f);

// 停止追踪
Hunter->StopFollowing();
```

## 8.4 Energy System (电量系统)

RobotDog 拥有电量系统，移动时消耗电量，可在充电站充电。

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `Energy` | 100.f | 当前电量 (0-100) |
| `MaxEnergy` | 100.f | 最大电量 |
| `EnergyDrainRate` | 1.f | 每秒消耗电量 |

### 头顶显示

电量会实时显示在机器人头顶：
- 绿色: Energy >= 50%
- 黄色: 20% <= Energy < 50%
- 红色: Energy < 20% (低电量警告)

### 使用示例

```cpp
AMARobotDogCharacter* Robot = ...;

// 检查电量
if (Robot->HasEnergy())
{
    Robot->TryNavigateTo(Target);
}

// 获取电量百分比
float Percent = Robot->GetEnergyPercent();

// 恢复电量
Robot->RestoreEnergy(50.f);
```

## 8.5 ChargingStation (充电站) & 渐进式充电

充电站是一个可放置在关卡中的 Actor，机器人进入范围后可触发充电。

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `InteractionRadius` | 300.f | 交互范围 |

### 渐进式充电机制

充电采用渐进式恢复，而非瞬间充满：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `ChargeRate` | 20.f | 每秒恢复电量百分比 |
| `ChargeInterval` | 0.5f | 充电更新间隔（秒） |

```
充电过程:
0s   → 20%  (初始)
0.5s → 30%  (+10%)
1.0s → 40%  (+10%)
...
4.0s → 100% (充满，自动结束)
```

### 使用示例

```cpp
// 在关卡中放置 ChargingStation
// 机器人进入范围后:
Robot->TryCharge();  // 开始渐进式充电

// 离开充电站范围会中断充电
// 电量充满后自动结束充电
```

## 8.6 PatrolPath (巡逻路径)

巡逻路径定义了机器人巡逻的路径点序列。

### 使用示例

```cpp
// 方式1: 直接使用路径点数组
TArray<FVector> Waypoints = {
    FVector(0, 0, 0),
    FVector(500, 0, 0),
    FVector(500, 500, 0),
    FVector(0, 500, 0)
};
Robot->TryPatrol(Waypoints);

// 方式2: 使用 PatrolPath Actor
AMAPatrolPath* Path = ...;  // 在关卡中放置
Robot->TryPatrolPath(Path);

// 停止巡逻
Robot->StopPatrol();
```

## 8.7 GA_Search (搜索技能)

使用摄像头检测视野内的 Human 角色。

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `DetectionInterval` | 0.5f | 检测间隔（秒） |
| `DetectionRange` | 1000.f | 检测范围 |
| `DetectionFOV` | 90.f | 检测视野角度（度） |

### 使用示例

```cpp
// 需要附着 CameraSensor
Robot->TrySearch();

// 发现 Human 时:
// - 输出日志 "Find Target"
// - 屏幕显示 "[RobotDog] Find Target: Human"
// - 触发 Event.Target.Found 事件

// 停止搜索
Robot->StopSearch();
```

## 8.8 GA_Formation (编队技能)

多机器人按阵型跟随 Leader 移动。

### 编队类型

| 类型 | 说明 |
|------|------|
| `Line` | 一字排开（左右展开） |
| `Column` | 纵队（前后排列） |
| `Wedge` | 楔形（V形阵型） |
| `Diamond` | 菱形 |

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `FormationSpacing` | 200.f | 编队间距 |
| `UpdateInterval` | 0.3f | 位置更新间隔 |

### 使用示例

```cpp
AMACharacter* Leader = ...;
AMARobotDogCharacter* Robot1 = ...;
AMARobotDogCharacter* Robot2 = ...;

// Robot1 加入楔形编队，位置 0
Robot1->GetAbilitySystemComponent()->TryActivateFormation(Leader, EFormationType::Wedge, 0);

// Robot2 加入楔形编队，位置 1
Robot2->GetAbilitySystemComponent()->TryActivateFormation(Leader, EFormationType::Wedge, 1);

// 取消编队
Robot1->GetAbilitySystemComponent()->CancelFormation();
```

## 8.9 GA_Avoid (避障技能)

检测障碍物并计算避障路径。

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `DetectionRadius` | 200.f | 障碍物检测半径 |
| `AvoidanceStrength` | 1.f | 避障力度 |
| `CheckInterval` | 0.1f | 检测间隔 |

### 使用示例

```cpp
// 启动避障并设置目标
Robot->GetAbilitySystemComponent()->TryActivateAvoid(TargetLocation);

// 取消避障
Robot->GetAbilitySystemComponent()->CancelAvoid();
```

## 8.10 GA_Report (报告技能)

在屏幕上显示报告对话框。

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `DisplayDuration` | 5.f | 显示时长（秒） |

### 使用示例

```cpp
// 发送报告
Robot->GetAbilitySystemComponent()->TryActivateReport(TEXT("发现可疑目标"));

// 屏幕显示:
// [Report from RobotDog]
// 发现可疑目标
```

## 8.11 GA_Observe (观察技能)

Sensor 持续观察目标或区域。

### 属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `ObserveRange` | 1500.f | 观察范围 |
| `UpdateInterval` | 0.3f | 更新间隔 |

### 使用示例

```cpp
// 观察特定目标
Robot->GetAbilitySystemComponent()->TryActivateObserve(TargetActor);

// 区域观察（无特定目标）
Robot->GetAbilitySystemComponent()->TryActivateObserve(nullptr);

// 目标离开范围时触发 Event.Target.Lost
```

## 9. State Tree 模块详解

### 9.1 概述

State Tree 是 UE5 的新一代行为树系统，用于管理 Character 的高层状态决策。本项目使用 State Tree + GAS 的组合架构：

- **State Tree**: 决定"做什么"（状态切换）
- **GAS**: 决定"怎么做"（技能执行）

### 9.2 自定义 Task (CARLA 风格)

Task 不存储可配置参数，而是从 Robot 获取：

| Task | 功能 | 从 Robot 获取的参数 |
|------|------|---------------------|
| `MASTTask_Patrol` | 循环巡逻路径点 | `PatrolPath`, `ScanRadius`(到达判定) |
| `MASTTask_Navigate` | 导航到指定位置 | - |
| `MASTTask_Charge` | 自动充电 | `ChargeRate` |
| `MASTTask_Coverage` | 区域覆盖扫描 | `CoverageArea`, `ScanRadius`(路径间距) |
| `MASTTask_Follow` | 跟随目标 | `FollowTarget`, `ScanRadius`(跟随距离) |
| `MASTTask_ActivateAbility` | 激活 GAS Ability | AbilityTag, WaitForAbilityEnd |

```cpp
// Task 实现示例 (MASTTask_Follow)
EStateTreeRunStatus FMASTTask_Follow::EnterState()
{
    // 从 Robot 获取参数，而非 Task 内部配置
    AMACharacter* Target = Robot->GetFollowTarget();
    float Distance = Robot->ScanRadius;
    
    Robot->TryFollowActor(Target, Distance);
    return EStateTreeRunStatus::Running;
}
```

### 9.3 自定义 Condition

| Condition | 功能 | 参数 |
|-----------|------|------|
| `MASTCondition_HasCommand` | 检查是否收到指定命令 | CommandTag |
| `MASTCondition_LowEnergy` | 检查电量 < 阈值 | EnergyThreshold (默认 20%) |
| `MASTCondition_FullEnergy` | 检查电量 >= 阈值 | FullThreshold (默认 100%) |
| `MASTCondition_HasPatrolPath` | 检查是否有可用巡逻路径 | PatrolPathTag |

### 9.4 命令系统

通过 Gameplay Tags 发送命令，State Tree Condition 检测命令并触发状态转换：

```cpp
// 发送命令（需要先清除其他命令 Tag）
UAbilitySystemComponent* ASC = Robot->GetAbilitySystemComponent();

// 清除所有命令 Tag
ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Patrol")));
ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Follow")));
ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Coverage")));
// ... 清除其他命令

// 添加新命令
ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Patrol")));

// State Tree 中的 MASTCondition_HasCommand 会检测到命令
// 触发状态转换到 Patrol 状态
```

**重要**: 发送新命令时必须清除所有其他命令 Tag，否则旧状态的 Enter Condition 仍然满足，导致状态不切换。

### 9.5 状态转换示例

```
┌──────────────┐  Command.Patrol   ┌──────────────┐
│     Idle     │ ────────────────► │    Patrol    │
└──────────────┘                   └──────┬───────┘
       ▲                                  │
       │ Command.Idle                     │ LowEnergy (<20%)
       │                                  ▼
       │                           ┌──────────────┐
       └────────────────────────── │    Charge    │
              FullEnergy (100%)    └──────────────┘
```

### 9.6 配置要点

1. **Schema**: 必须选择 `StateTree AI Component Schema`
2. **AIController**: Character 必须有 AIController
3. **Actor Tag**: 使用 Actor Tag 在运行时查找场景中的 Actor（如 PatrolPath）

---

## 10. 输入系统 (Enhanced Input)

### 9.1 架构

使用 UE5 Enhanced Input System，支持数据驱动的输入配置：

```
┌─────────────────────────────────────────────────────────────┐
│                 Enhanced Input 架构                          │
│                                                             │
│  Input Mapping Context (IMC)                                │
│  └── 定义按键到 InputAction 的映射                           │
│                                                             │
│  Input Actions (IA)                                         │
│  └── 定义输入动作（点击、按键等）                             │
│                                                             │
│  MAInputConfig (Data Asset)                                 │
│  └── 定义 InputAction 到 GameplayTag 的映射                  │
│                                                             │
│  MAPlayerController                                         │
│  └── 绑定 InputAction 到具体函数                             │
└─────────────────────────────────────────────────────────────┘
```

### 9.2 自动配置

**无需手动创建资产！** 所有 Input Actions 和 Mapping Context 在 C++ 中自动创建：

```cpp
// MAInputActions.h - 单例模式，自动初始化
UMAInputActions* InputActions = UMAInputActions::Get();

// 包含所有预定义的 Input Actions:
InputActions->IA_LeftClick
InputActions->IA_RightClick
InputActions->IA_Pickup
// ...

// 以及默认的 Mapping Context:
InputActions->DefaultMappingContext
```

PlayerController 在 BeginPlay 时自动添加 Mapping Context，无需额外配置。

### 9.3 默认按键映射

| 按键 | Input Action | 功能 |
|-----|--------------|------|
| **左键** | IA_LeftClick | 移动所有 Human Character |
| **右键** | IA_RightClick | 移动所有 RobotDog |
| **P** | IA_Pickup | 所有 Human 尝试拾取 |
| **O** | IA_Drop | 所有 Human 放下物品 |
| **I** | IA_SpawnItem | 生成可拾取方块 |
| **T** | IA_SpawnRobotDog | 生成机器狗 |
| **Y** | IA_PrintInfo | 打印 Actor 信息 |
| **U** | IA_DestroyLast | 销毁最后一个 Character |
| **Tab** | IA_SwitchCamera | 切换 Camera 视角 |
| **0** | IA_ReturnSpectator | 返回上帝视角 |

### 9.4 相关文件

```
Input/
├── MAInputConfig.h/cpp      # 输入配置数据资产
└── MAInputComponent.h/cpp   # 增强输入组件（可选）
```

## 11. 头顶状态显示

每个 Character 头顶可以显示当前正在执行的技能名称和参数，便于调试和观察。

使用 `DrawDebugString` 实现，**仅在 Development/Debug 版本中显示**。

```cpp
// 显示状态（持续指定时间）
Character->ShowStatus(TEXT("[Navigate] → (100, 200)"), 3.0f);

// 显示技能状态（便捷方法）
Character->ShowAbilityStatus(TEXT("Navigate"), TEXT("→ (100, 200)"));

// 清除显示
Character->ShowStatus(TEXT(""), 0.f);
```

## 12. 开发路线

### Phase 1: 核心框架 ✅ 已完成
- [x] UMAActorSubsystem - Character/Sensor 生命周期管理
- [x] AMACharacter 及其子类
- [x] AMASensor 及其子类
- [x] GAS 集成 - AbilitySystemComponent
- [x] GA_Pickup / GA_Drop - 拾取/放下技能
- [x] GA_Navigate - 导航技能
- [x] GA_Follow - 追踪技能
- [x] GA_TakePhoto - 拍照技能
- [x] AMAPickupItem - 可拾取物品
- [x] State Tree 基础集成

### Phase 2: 输入与交互 ✅ 已完成
- [x] Enhanced Input System - 替换旧版输入系统
- [x] Energy System - 机器人电量系统
- [x] ChargingStation - 充电站
- [x] PatrolPath - 巡逻路径
- [x] GA_Search - 搜索技能（检测 Human）
- [x] GA_Observe - 观察技能
- [x] GA_Report - 报告技能
- [x] GA_Charge - 充电技能
- [x] GA_Formation - 编队技能
- [x] GA_Avoid - 避障技能

### Phase 3: State Tree 集成 ✅ 已完成
- [x] MAStateTreeComponent - State Tree 组件
- [x] MASTTask_Patrol - 巡逻 Task
- [x] MASTTask_Navigate - 导航 Task
- [x] MASTTask_ActivateAbility - 激活 Ability Task
- [x] MASTCondition_HasCommand - 命令检测 Condition
- [x] MASTCondition_LowEnergy - 低电量检测 Condition
- [x] MASTCondition_FullEnergy - 满电量检测 Condition
- [x] MASTCondition_HasPatrolPath - 巡逻路径检测 Condition

### Phase 4: 交互与感知 🔄 进行中
- [ ] GA_Interact - 交互技能
- [ ] 更多 Sensor 类型 (Lidar, Depth, IMU)

### Phase 5: 关系管理
- [ ] UMARelationSubsystem - 实体关系图
- [ ] 关系类型枚举
- [ ] 关系查询 API

### Phase 6: 持久化系统
- [ ] UMAGameInstance - 跨关卡持久数据
- [ ] SaveGame 系统 - 游戏存档/读档
- [ ] 关卡切换与数据保持

### 未来规划
| 模块 | 说明 | 优先级 |
|------|------|--------|
| **Enhanced Input** 完成 | UE5 新输入系统，支持复杂输入映射 | 高 |
| **GameInstance** | 跨关卡持久数据（玩家进度、设置等） | 中 |
| **SaveGame** | 存档/读档系统 | 中 |
| **UI/HUD** | 用户界面（队友负责，解耦设计） | 低 |

## 13. 重要设计原则

### 12.1 虚函数多态设计

不同类型的 Character 可能需要不同的行为实现。使用 C++ 虚函数实现多态：

```cpp
// MACharacter.h (基类)
class AMACharacter
{
protected:
    // 导航期间每帧调用，子类可重写
    virtual void OnNavigationTick();
};

// MAHumanCharacter.cpp (子类重写)
void AMAHumanCharacter::OnNavigationTick()
{
    // Human 特有：为动画蓝图提供加速度输入
    FVector Velocity = GetCharacterMovement()->Velocity;
    if (Velocity.Size2D() > 3.0f)
    {
        GetCharacterMovement()->AddInputVector(Velocity.GetSafeNormal2D());
    }
}
```

### 12.2 物理组件层级规则

对于有物理模拟的可拾取物品，**必须将物理组件设为 RootComponent**：

```cpp
// ✅ 正确：MeshComponent 作为 Root
MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
RootComponent = MeshComponent;
MeshComponent->SetSimulatePhysics(true);

CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
CollisionComponent->SetupAttachment(RootComponent);  // 检测球跟着 Mesh 走
```
