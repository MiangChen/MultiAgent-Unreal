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

- **State Tree**: 高层策略决策，管理 Agent 的主模式（探索/拍照/交互）
- **GAS**: 具体技能执行，处理动画、冷却、消耗等
- **Gameplay Tags**: 两者之间的通信桥梁

## 3. 当前文件结构

```
unreal_project/Source/MultiAgent/
├── AgentManager/                    # AgentManager 模块 (司命) ✅ 已实现
│   ├── MAAgentSubsystem.h/cpp       # Agent 管理子系统
│   ├── MAAgent.h/cpp                # Agent 基类 (集成 GAS)
│   ├── MAHumanAgent.h/cpp           # 人类 Agent
│   ├── MARobotDogAgent.h/cpp        # 机器狗 Agent
│   ├── MASensorAgent.h/cpp          # 传感器 Agent 基类
│   └── MACameraAgent.h/cpp          # 摄像头 Agent（传感器）
├── GAS/                             # GAS 模块 ✅ 已实现
│   ├── MAAbilitySystemComponent.h/cpp  # GAS 核心组件
│   ├── MAGameplayTags.h/cpp         # Gameplay Tags 定义
│   ├── MAGameplayAbilityBase.h/cpp  # Ability 基类
│   └── Abilities/                   # 具体技能
│       ├── GA_Pickup.h/cpp          # 拾取技能
│       └── GA_Drop.h/cpp            # 放下技能
├── StateTree/                       # State Tree 模块 ✅ 已实现
│   ├── MAStateTreeComponent.h/cpp   # State Tree 组件
│   └── Tasks/                       # State Tree Tasks
│       └── MASTTask_ActivateAbility.h/cpp  # 激活 Ability 的 Task
├── Interaction/                     # 交互模块 ✅ 已实现
│   └── MAPickupItem.h/cpp           # 可拾取物品
├── Core/                            # 核心框架
│   ├── MAGameMode.h/cpp             # 游戏模式
│   └── MAPlayerController.h/cpp     # 玩家控制器
└── MultiAgent.h/cpp                 # 模块定义
```

## 4. Manager 子系统设计

### 4.1 全局 Manager vs Agent 组件

本项目的"司XXX"分为两个层级：

```
┌─────────────────────────────────────────────────────────────┐
│                    全局 Manager 层                           │
│                  UGameInstanceSubsystem                      │
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │ 司命        │  │ 司缘        │  │ 司图        │         │
│  │AgentManager │  │RelationMgr  │  │ MapManager  │         │
│  │ 管理所有    │  │ 管理Agent   │  │ 管理地图    │         │
│  │ Agent生命周期│  │ 之间的关系  │  │ 导航感知    │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ 管理
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Agent 组件层                              │
│                  每个 Agent 独立拥有                         │
│                                                             │
│     Agent #1              Agent #2              Agent #3    │
│  ┌───────────┐         ┌───────────┐         ┌───────────┐ │
│  │ 司脑      │         │ 司脑      │         │ 司脑      │ │
│  │StateTree  │         │StateTree  │         │StateTree  │ │
│  │ 决策      │         │ 决策      │         │ 决策      │ │
│  ├───────────┤         ├───────────┤         ├───────────┤ │
│  │ 司能      │         │ 司能      │         │ 司能      │ │
│  │ ASC       │         │ ASC       │         │ ASC       │ │
│  │ 技能执行  │         │ 技能执行  │         │ 技能执行  │ │
│  └───────────┘         └───────────┘         └───────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 4.2 Manager 列表

| Manager | 中文名 | 层级 | 功能介绍 | UE/C++ 实现方案 | 状态 |
|---------|-------|------|---------|----------------|------|
| **AgentManager** | 司命 | 全局 | 智能体生命周期管理 | `UMAAgentSubsystem` | ✅ 已实现 |
| **RelationManager** | 司缘 | 全局 | 智能体关系管理 | `UMARelationSubsystem` | ❌ 待开发 |
| **MapManager** | 司图 | 全局 | 地图感知 | `UNavigationSystemV1` | ❌ 待开发 |
| **StateTree** | 司脑 | Agent级 | 状态决策 | `UMAStateTreeComponent` | ✅ 已实现 |
| **ASC** | 司能 | Agent级 | 技能执行 | `UMAAbilitySystemComponent` | ✅ 已实现 |

### 4.3 司能 (ASC) 详解

**司能** 是 UE GAS (Gameplay Ability System) 的核心组件 ASC (Ability System Component) 的中文名。

```
┌─────────────────────────────────────────────────────────────┐
│                    GAS 框架 (UE 内置)                        │
│                 Gameplay Ability System                      │
│                                                             │
│  包含：UAbilitySystemComponent, UGameplayAbility,           │
│        UGameplayEffect, FGameplayTag 等                     │
└─────────────────────────────────────────────────────────────┘
                         │
                         │ 继承
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                 项目代码 (类定义)                             │
│                                                             │
│  • UMAAbilitySystemComponent (司能组件)                     │
│  • UMAGameplayAbilityBase (技能基类)                        │
│  • GA_Navigate, GA_Pickup, GA_Drop... (具体技能)            │
└─────────────────────────────────────────────────────────────┘
                         │
                         │ 实例化
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                   运行时 (每个Agent一个实例)                  │
│                                                             │
│  Human Agent  ──► 司能实例 ──► [GA_Navigate, GA_Pickup...]  │
│  Drone Agent  ──► 司能实例 ──► [GA_Navigate, GA_TakePhoto...│
│  RobotDog     ──► 司能实例 ──► [GA_Navigate, GA_Patrol...]  │
└─────────────────────────────────────────────────────────────┘
```

**关键概念：**
- **GAS**: UE 引擎内置的技能框架，全局唯一
- **ASC (司能)**: 挂载在每个 Agent 上的组件实例，管理该 Agent 的技能
- **GA_XXX**: 技能类定义（代码），可被多个 Agent 共用
- **技能实例**: 运行时创建，每个 Agent 的 ASC 中有自己的技能实例

### 4.4 技能激活方式：Handle vs Tag

司能 (ASC) 支持两种技能激活/取消方式：

```
┌─────────────────────────────────────────────────────────────┐
│                    技能激活调用链                            │
│                                                             │
│  PlayerController ──► Agent ──► ASC ──► GA_XXX             │
│       (Core)        (司命)    (司能)   (技能)               │
│                                                             │
│  方式1: Handle (直接激活)                                   │
│  Agent.TryNavigateTo() → ASC.TryActivateAbility(Handle)    │
│                                                             │
│  方式2: Tag (通用激活)                                      │
│  StateTree → ASC.TryActivateAbilityByTag(Tag)              │
└─────────────────────────────────────────────────────────────┘
```

| 调用者 | 方法 | 内部实现 | 说明 |
|-------|------|---------|------|
| Agent | `TryNavigateTo()` | Handle | 精确、直接 |
| Agent | `TryPickup()` | Handle | 精确、直接 |
| Agent | `TryDrop()` | Handle | 精确、直接 |
| Agent | `CancelNavigation()` | Handle | 精确取消 |
| StateTree | `TryActivateAbilityByTag()` | Tag | 通用，按 Tag 匹配 |
| StateTree | `CancelAbilityByTag()` | Tag | 通用，按 Tag 匹配 |

**设计原则：**
- **Handle 是司能内部实现细节**，外部调用者不需要知道
- **Agent 层提供语义化接口**（如 `TryNavigateTo`），内部使用 Handle
- **StateTree 使用 Tag**，因为它不知道具体的 Handle，需要通过 Tag 匹配

## 5. 司能 (GAS) 模块详解

### 5.1 Gameplay Tags 定义

```cpp
// State Tags (State Tree 状态)
State.Exploration          // 探索模式
State.PhotoMode            // 拍照模式
State.Interaction          // 交互模式
State.Interaction.Pickup   // 拾取中
State.Interaction.Dialogue // 对话中

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

### 5.2 Ability 列表

| Ability | 功能 | 激活条件 | 阻止条件 |
|---------|------|---------|---------|
| `GA_Pickup` | 拾取物品 | 附近有可拾取物品 | Status.Holding |
| `GA_Drop` | 放下物品 | Status.Holding | - |
| `GA_Navigate` | 导航移动 | 有 AIController | - |

### 5.3 使用示例

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

## 6. State Tree 模块详解

### 6.1 State Tree 与 GAS 协作流程

```
1. State Tree 进入 Pickup 状态
   ↓
2. MASTTask_ActivateAbility 激活 GA_Pickup
   ↓
3. GA_Pickup 执行拾取逻辑，发送 Event.Pickup.End
   ↓
4. State Tree 监听事件，切换回 Exploration 状态
```

### 6.2 自定义 State Tree Task

```cpp
// 在 State Tree 中使用 MA Activate Ability Task
USTRUCT(meta = (DisplayName = "MA Activate Ability"))
struct FMASTTask_ActivateAbility : public FStateTreeTaskCommonBase
{
    UPROPERTY(EditAnywhere)
    FGameplayTag AbilityTag;  // 要激活的 Ability
    
    UPROPERTY(EditAnywhere)
    bool bWaitForAbilityEnd;  // 是否等待完成
};
```

## 7. 类继承关系

```
UAbilitySystemComponent (UE GAS)
    └── UMAAbilitySystemComponent

UGameplayAbility (UE GAS)
    └── UMAGameplayAbilityBase
            ├── UGA_Pickup
            └── UGA_Drop

UStateTreeComponent (UE StateTree)
    └── UMAStateTreeComponent

ACharacter (UE) + IAbilitySystemInterface
    └── AMAAgent (集成 GAS)
            ├── AMAHumanAgent
            └── AMARobotDogAgent

AActor (UE)
    └── AMAPickupItem (可拾取物品)
```

## 8. Camera 系统

### 8.1 Camera Agent 继承体系

```
AMAAgent (基类)
└── AMASensorAgent (传感器基类，支持 Attach)
    └── AMACameraAgent (摄像头，支持拍照)
```

### 8.2 第三人称视角

每个 Human/RobotDog 生成时会自动附着一个第三人称摄像头：

| Agent | 摄像头位置 | 说明 |
|-------|-----------|------|
| Human | `(-200, 0, 100)` Pitch -10° | 后方 2m，高 1m，向下看 |
| RobotDog | `(-150, 0, 80)` Pitch -15° | 后方 1.5m，高 0.8m |

### 8.3 上帝视角

玩家默认使用 UE 原生 `SpectatorPawn`，支持：
- 鼠标右键/中键按住 + 拖动：旋转视角
- WASD：移动
- Q/E：上下飞行

## 9. 测试按键

| 按键 | 功能 |
|-----|------|
| **Tab** | 循环切换 Agent 的第三人称视角 |
| **0** | 返回上帝视角 |
| **T** | 生成一个机器狗 |
| **Y** | 打印当前所有 Agent 信息 |
| **U** | 销毁最后一个 Agent |
| **I** | 在鼠标位置生成可拾取方块 |
| **P** | 所有 Human Agent 尝试拾取 |
| **O** | 所有 Human Agent 放下物品 |
| 左键 | 移动所有 Human Agent |
| 右键 | 移动所有 Agent |

## 10. 追踪功能 (Ground Truth Tracking)

### 10.1 概述

本项目实现了基于 Ground Truth 的追踪功能，让一个 Agent 可以持续跟随另一个 Agent。

**Ground Truth 追踪**：直接使用仿真环境中的真实位置数据，不需要视觉检测算法。

```
┌─────────────────────────────────────────────────────────────┐
│                    追踪工作原理                              │
│                                                             │
│  每 0.5 秒:                                                 │
│    1. 获取目标位置 (Ground Truth)                           │
│       TargetLocation = TargetAgent->GetActorLocation()     │
│                                                             │
│    2. 计算跟随位置 (保持距离)                                │
│       FollowLocation = TargetLocation + Direction * Dist   │
│                                                             │
│    3. 导航过去                                              │
│       AIController->MoveToLocation(FollowLocation)         │
└─────────────────────────────────────────────────────────────┘
```

### 10.2 GA_Follow 技能

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `FollowDistance` | 200.f | 与目标保持的距离 |
| `UpdateInterval` | 0.5f | 位置更新频率（秒） |
| `Repath_Threshold` | 100.f | 目标移动超过此距离才重新导航 |

### 10.3 使用方式

```cpp
// 获取两个 Agent
AMAAgent* Hunter = AgentSubsystem->GetAgentByID(0);
AMAAgent* Target = AgentSubsystem->GetAgentByID(1);

// Hunter 开始追踪 Target，保持 300 单位距离
Hunter->TryFollowAgent(Target, 300.f);

// 停止追踪
Hunter->StopFollowing();
```

### 10.4 演示场景

游戏启动后自动生成：
- 2 个 Human（左键控制移动）
- 3 个 RobotDog（右键控制移动）
- 1 个 Tracker_Dog（自动追踪第一个 Human）

| 操作 | 效果 |
|------|------|
| **左键** | 移动所有 Human Agent |
| **右键** | 移动普通 RobotDog（不包括 Tracker_Dog） |
| **自动** | Tracker_Dog 持续追踪 Human_0 |

### 10.5 与视觉追踪的对比

| 特性 | Ground Truth 追踪 | 视觉追踪 |
|------|------------------|----------|
| 准确性 | 100% | 取决于检测算法 |
| 遮挡处理 | 无影响 | 会丢失目标 |
| 计算开销 | 极低 | 高（需要 CV 算法） |
| 真实性 | 低（作弊） | 高（模拟真实） |
| 适用场景 | 仿真测试、数据采集 | 真实机器人部署 |

### 10.6 导航失败处理

当跟随位置不在 NavMesh 上时（如目标在高台边缘），GA_Follow 会自动 fallback：

```
1. 尝试导航到计算的跟随位置
   ↓ 失败
2. Fallback: 直接导航到目标位置（用 FollowDistance 作为接受半径）
   ↓ 失败
3. 显示 "Cannot reach!" 提示
```

### 10.7 扩展：视觉追踪

如需更真实的追踪行为，可以扩展 GA_Follow 添加：
- 视野检测（目标是否在视野内）
- 遮挡检测（是否被墙挡住）
- 丢失目标后的搜索行为

## 11. 头顶状态显示

### 11.1 概述

每个 Agent 头顶可以显示当前正在执行的技能名称和参数，便于调试和观察。

使用 `DrawDebugString` 实现，**仅在 Development/Debug 版本中显示，Shipping 版本自动剔除**。

### 11.2 使用方式

```cpp
// 显示状态（持续指定时间）
Agent->ShowStatus(TEXT("[Navigate] → (100, 200)"), 3.0f);

// 显示技能状态（便捷方法）
Agent->ShowAbilityStatus(TEXT("Navigate"), TEXT("→ (100, 200)"));

// 清除显示
Agent->ShowStatus(TEXT(""), 0.f);
```

### 11.3 各技能的显示行为

| 技能 | 显示时机 | 清除时机 |
|------|---------|---------|
| **Navigate** | 开始移动时 | 到达目标后 |
| **Follow** | 开始追踪/需要移动时 | 到达跟随距离内（1.1倍） |
| **Pickup** | 拾取时 | 3秒后自动消失 |
| **Drop** | 放下时 | 3秒后自动消失 |

### 11.4 配置

```cpp
// MAAgent.h
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
bool bShowStatusAboveHead = true;  // 是否启用头顶状态显示
```

## 12. 开发路线

### Phase 1: 核心框架 ✅ 已完成
- [x] UMAAgentSubsystem - Agent 生命周期管理
- [x] AMAAgent 及其子类
- [x] GAS 集成 - AbilitySystemComponent
- [x] GA_Pickup / GA_Drop - 拾取/放下技能
- [x] AMAPickupItem - 可拾取物品
- [x] State Tree 基础集成

### Phase 2: 扩展技能
- [x] GA_TakePhoto - 拍照技能 ✅ 已实现
- [x] GA_Navigate - 导航技能 ✅ 已实现
- [x] GA_Follow - 追踪技能 ✅ 已实现
- [x] MACameraAgent - 传感器摄像头 ✅ 已实现
- [ ] GA_Interact - 交互技能

### Phase 3: State Tree 完善
- [ ] 完整的状态机配置
- [ ] 状态转换条件
- [ ] AI 决策逻辑

### Phase 4: 关系管理
- [ ] UMARelationSubsystem - 实体关系图
- [ ] 关系类型枚举
- [ ] 关系查询 API

## 13. 重要设计原则

### 13.1 虚函数多态设计

不同类型的 Agent 可能需要不同的行为实现。使用 C++ 虚函数实现多态，避免类型判断：

```cpp
// MAAgent.h (基类)
class AMAAgent
{
protected:
    // 导航期间每帧调用，子类可重写
    virtual void OnNavigationTick();
};

// MAHumanAgent.cpp (子类重写)
void AMAHumanAgent::OnNavigationTick()
{
    // Human 特有：为动画蓝图提供加速度输入
    FVector Velocity = GetCharacterMovement()->Velocity;
    if (Velocity.Size2D() > 3.0f)
    {
        GetCharacterMovement()->AddInputVector(Velocity.GetSafeNormal2D());
    }
}

// MADroneAgent (不重写，使用基类空实现)
```

**当前虚函数列表：**

| 虚函数 | 基类行为 | Human 重写 | Drone 重写 |
|-------|---------|-----------|-----------|
| `OnNavigationTick()` | 空实现 | 添加加速度驱动动画 | 不重写 |

**设计原则：**
- 基类定义接口，提供默认空实现
- 子类按需重写，实现特定行为
- 避免在代码中写 `if (AgentType == Human)` 这种判断

### 13.2 物理组件层级规则

对于有物理模拟的可拾取物品，**必须将物理组件设为 RootComponent**：

```cpp
// ✅ 正确：MeshComponent 作为 Root
MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
RootComponent = MeshComponent;
MeshComponent->SetSimulatePhysics(true);

CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
CollisionComponent->SetupAttachment(RootComponent);  // 检测球跟着 Mesh 走

// ❌ 错误：检测球作为 Root，Mesh 作为子组件
// 会导致 Mesh 掉落后检测球留在原地
```

**原因**：
- `AttachToComponent()` 只移动 Actor 的 RootComponent
- 有物理模拟的子组件会被物理引擎独立控制位置
- 详见 `doc/bug.md` Bug #1
