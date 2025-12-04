# MultiAgent-Unreal 架构设计

## 1. 系统概述

MultiAgent-Unreal 是一个基于 Unreal Engine 5 的多智能体仿真框架，用于机器人、无人机、角色等异构智能体的协同仿真。

## 2. 核心架构：State Tree + GAS

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
│   └── MAPickupItem.h/cpp           # 可拾取物品
├── GAS/                             # GAS 模块 ✅ 已实现
│   ├── MAAbilitySystemComponent.h/cpp  # GAS 核心组件
│   ├── MAGameplayTags.h/cpp         # Gameplay Tags 定义
│   ├── MAGameplayAbilityBase.h/cpp  # Ability 基类
│   └── Ability/                     # 具体技能
│       ├── GA_Pickup.h/cpp          # 拾取技能
│       ├── GA_Drop.h/cpp            # 放下技能
│       ├── GA_Navigate.h/cpp        # 导航技能
│       ├── GA_Follow.h/cpp          # 追踪技能
│       └── GA_TakePhoto.h/cpp       # 拍照技能
├── Input/                           # 输入系统 ✅ 已实现
│   ├── MAInputActions.h/cpp         # Input Actions 单例 (运行时创建)
│   ├── MAInputConfig.h/cpp          # 输入配置数据资产
│   └── MAInputComponent.h/cpp       # 增强输入组件
├── StateTree/                       # State Tree 模块 ✅ 已实现
│   ├── MAStateTreeComponent.h/cpp   # State Tree 组件
│   └── Task/                        # State Tree Task
│       └── MASTTask_ActivateAbility.h/cpp  # 激活 Ability 的 Task
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
    └── AMAPickupItem (可拾取物品)

UAbilitySystemComponent (UE GAS)
    └── UMAAbilitySystemComponent

UGameplayAbility (UE GAS)
    └── UMAGameplayAbilityBase
            ├── UGA_Pickup
            ├── UGA_Drop
            ├── UGA_Navigate
            ├── UGA_Follow
            └── UGA_TakePhoto

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

## 9. 输入系统 (Enhanced Input)

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

## 10. 头顶状态显示

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

## 11. 开发路线

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

### Phase 2: 输入与交互 🔄 进行中
- [x] Enhanced Input System - 替换旧版输入系统
- [ ] GA_Interact - 交互技能
- [ ] 更多 Sensor 类型 (Lidar, Depth, IMU)
- [ ] 完整的 State Tree 状态机配置

### Phase 3: 关系管理
- [ ] UMARelationSubsystem - 实体关系图
- [ ] 关系类型枚举
- [ ] 关系查询 API

### Phase 4: 持久化系统
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

## 12. 重要设计原则

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
