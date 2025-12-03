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
│   └── MACameraAgent.h/cpp          # 摄像头 Agent
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

| Manager | 中文名 | 功能介绍 | UE/C++ 实现方案 | 状态 |
|---------|-------|---------|----------------|------|
| **AgentManager** | 司命 | 智能体生命周期管理 | `UMAAgentSubsystem` | ✅ 已实现 |
| **GAS** | 司技 | 技能系统 | `UMAAbilitySystemComponent` + `GA_XXX` | ✅ 已实现 |
| **StateTree** | 司脑 | 状态决策 | `UMAStateTreeComponent` | ✅ 已实现 |
| **RelationManager** | 司缘 | 智能体关系管理 | `UMARelationSubsystem` | ❌ 待开发 |
| **MapManager** | 司图 | 地图感知 | `UNavigationSystemV1` | ❌ 待开发 |

## 5. GAS 模块详解

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
| `GA_Pickup` | 拾取物品 | Status.CanPickup | Status.Holding |
| `GA_Drop` | 放下物品 | Status.Holding | - |

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

## 8. 测试按键

| 按键 | 功能 |
|-----|------|
| **T** | 生成一个机器狗 |
| **Y** | 打印当前所有 Agent 信息 |
| **U** | 销毁最后一个 Agent |
| **I** | 在鼠标位置生成可拾取方块 |
| **P** | 所有 Human Agent 尝试拾取 |
| **O** | 所有 Human Agent 放下物品 |
| 左键 | 移动所有 Human Agent |
| 右键 | 移动所有 Agent |

## 9. 开发路线

### Phase 1: 核心框架 ✅ 已完成
- [x] UMAAgentSubsystem - Agent 生命周期管理
- [x] AMAAgent 及其子类
- [x] GAS 集成 - AbilitySystemComponent
- [x] GA_Pickup / GA_Drop - 拾取/放下技能
- [x] AMAPickupItem - 可拾取物品
- [x] State Tree 基础集成

### Phase 2: 扩展技能
- [ ] GA_TakePhoto - 拍照技能
- [ ] GA_Navigate - 导航技能
- [ ] GA_Interact - 交互技能

### Phase 3: State Tree 完善
- [ ] 完整的状态机配置
- [ ] 状态转换条件
- [ ] AI 决策逻辑

### Phase 4: 关系管理
- [ ] UMARelationSubsystem - 实体关系图
- [ ] 关系类型枚举
- [ ] 关系查询 API

## 10. 重要设计原则

### 10.1 物理组件层级规则

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
