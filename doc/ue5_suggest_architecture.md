# UE5 Game Framework 架构解析

> 更新日期: 2025-12-13
> 重构: 引入 Capability Component 模式

## 整体流程

```
Engine → GameInstance → GameMode → World/Level → Actors/Pawns/Characters
```

---

## 架构图解析

### 1. Engine & Game Instance（黄色区域）

| 组件 | 作用 |
|------|------|
| **Engine** | UE5 引擎核心，管理所有子系统 |
| **GameInstance** | 全局单例，跨关卡存活，存储全局数据 |
| **GameMode** | 每个关卡的规则管理器，负责生成玩家、设置规则 |

### 2. Input & Controller（蓝色区域）

| 组件 | 作用 |
|------|------|
| **Input** | 输入系统（Enhanced Input） |
| **PlayerController** | 玩家控制器，处理输入→命令转换 |
| **AIController** | AI 控制器，用于 NPC/Agent 的自主行为 |

### 3. Behaviors（绿色区域）

| 组件 | 作用 |
|------|------|
| **Behavior Tree** | 传统 AI 行为树（UE4 风格） |
| **State Tree** | UE5 新的状态机，更灵活 |
| **Navigation** | 导航系统（NavMesh 寻路） |

### 4. GameState & World（青色区域）

| 组件 | 作用 |
|------|------|
| **GameState** | 游戏状态，所有玩家可见的共享数据 |
| **World** | 当前世界，包含所有 Actor |
| **Level** | 关卡，World 的子集 |

### 5. Pawn（红色区域）

| 组件 | 作用 |
|------|------|
| **Pawn** | 可被控制的实体基类 |
| **PlayerState** | 玩家状态（分数、名字等） |

### 6. Character（紫色区域）

| 组件 | 作用 |
|------|------|
| **Character** | Pawn 的子类，有移动能力 |
| **CharacterMovementComponent** | 移动组件，处理行走、跳跃、飞行 |
| **SkeletalMesh** | 骨骼网格，用于动画渲染 |
| **CapsuleComponent** | 碰撞胶囊，用于物理检测 |

---

## 核心概念："谁拥有谁" & "谁控制谁"

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         所有权层级                                       │
│                                                                         │
│  Engine                                                                 │
│    └── GameInstance (全局单例，跨关卡)                                   │
│          └── World                                                      │
│                ├── GameMode (关卡规则)                                   │
│                ├── GameState (共享状态)                                  │
│                ├── PlayerController (玩家控制器)                         │
│                │     └── PlayerState                                    │
│                └── Actors                                               │
│                      └── Pawn / Character (被控制实体)                   │
│                            ├── Components (功能组件)                     │
│                            └── AIController (AI 控制)                   │
└─────────────────────────────────────────────────────────────────────────┘
```

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         控制关系                                         │
│                                                                         │
│  Input System                                                           │
│       │                                                                 │
│       ▼                                                                 │
│  PlayerController ──possess──► Pawn/Character                           │
│       │                              │                                  │
│       │                              ▼                                  │
│       │                     CharacterMovementComponent                  │
│       │                              │                                  │
│       │                              ▼                                  │
│       │                        Physical Movement                        │
│       │                                                                 │
│  AIController ────possess────► Pawn/Character (NPC)                     │
│       │                              │                                  │
│       ▼                              ▼                                  │
│  Behavior Tree / State Tree    Navigation System                        │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 与本项目架构的对应关系

| UE5 官方组件 | 本项目实现 | 说明 |
|-------------|-----------|------|
| GameInstance | `MAGameInstance` | 全局配置（服务器地址等） |
| GameMode | `MAGameMode` | JSON 配置加载，Agent 生成 |
| PlayerController | `MAPlayerController` | RTS 风格输入处理 |
| Character | `MACharacter` | Agent 基类 |
| Components | Sensors, ASC, StateTree | 功能组件 |
| AIController | Agent 的 AIController | 自主行为控制 |
| State Tree | `MASTTask_*` | 高层策略决策 |
| WorldSubsystem | `MAAgentManager`, `MACommandManager` 等 | 全局管理器 |

---

## 本项目的扩展

在 UE5 标准架构基础上，本项目增加了：

```
Character (MACharacter)
    │
    ├── GAS (Gameplay Ability System)
    │   └── Abilities: Navigate, Patrol, Follow, Charge...
    │
    ├── State Tree
    │   └── Tasks: MASTTask_Patrol, MASTTask_Coverage...
    │
    ├── Sensors (Component 模式)
    │   └── Camera, LiDAR (future)...
    │
    └── Interfaces (能力抽象)
        └── IMAPatrollable, IMAFollowable, IMACoverable, IMAChargeable
```

### WorldSubsystem 扩展

```
WorldSubsystem
    │
    ├── MAAgentManager      # Agent 生命周期 + JSON 配置
    ├── MACommandManager    # RTS 命令分发
    ├── MASelectionManager  # 框选、编组
    ├── MASquadManager      # 编队管理
    ├── MAViewportManager   # 视角切换
    └── MAEmergencyManager  # 突发事件
```

### Capability Component 系统 (2025-12 重构)

类似 ROS 的 Parameter Server，但参数存储在 Agent 本地：

```
Agent (Character)
    │
    ├── UMAEnergyComponent    : IMAChargeable   # 能量系统
    │   └── Energy, MaxEnergy, DrainRate
    │
    ├── UMAPatrolComponent    : IMAPatrollable  # 巡逻能力
    │   └── PatrolPath, ScanRadius
    │
    ├── UMAFollowComponent    : IMAFollowable   # 跟随能力
    │   └── FollowTarget
    │
    └── UMACoverageComponent  : IMACoverable    # 覆盖扫描
        └── CoverageArea, ScanRadius
```

**优势:**
- 零代码重复: 能力逻辑只写一次，RobotDog/Drone 共用
- 热插拔: 运行时可添加/移除能力
- 松耦合: StateTree Task 只依赖 Interface，不依赖具体 Character 类型

**使用方式:**
```cpp
// StateTree Task 获取能力参数
if (IMAChargeable* Chargeable = GetCapabilityInterface<IMAChargeable>(Actor))
{
    float Energy = Chargeable->GetEnergy();
}

// CommandManager 设置参数
if (UMAPatrolComponent* PatrolComp = Agent->FindComponentByClass<UMAPatrolComponent>())
{
    PatrolComp->SetPatrolPath(Path);
}
```

---

## 设计原则

| 原则 | 说明 |
|------|------|
| **组件化** | 功能通过 Component 附加，而非继承 |
| **松耦合** | 通过 Interface 和 Gameplay Tags 通信 |
| **配置驱动** | JSON 配置 Agent，无需修改代码 |
| **层级分离** | 策略层 (StateTree) 与执行层 (GAS) 分离 |
