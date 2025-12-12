# MultiAgent-Unreal 架构设计

## 0. System Architecture (Science Robotics / TRO Style)

### Fig. 1: Unified Multi-Agent Simulation Framework

```
╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════╗
║                                                                                                              ║
║   COMMAND FLOW                              SYSTEM LAYERS                              DATA FLOW             ║
║   ════════════                              ═════════════                              ═════════             ║
║                                                                                                              ║
║   ┌──────────┐      ╔════════════════════════════════════════════════════════════════════════╗              ║
║   │ Operator │      ║  L5: HUMAN-ROBOT INTERFACE                                             ║              ║
║   │ [G] [F]  │─────▶║  ┌────────────────┐  ┌────────────────┐  ┌────────────────────────┐   ║◀──┐          ║
║   │ [H] [K]  │      ║  │ RTS Commands   │  │ Box Selection  │  │ TCP Video Feedback     │   ║   │          ║
║   └──────────┘      ║  │ Patrol/Follow/ │  │ Ctrl+1~5 Group │  │ Real-time Streaming    │   ║   │          ║
║                     ║  │ Charge/Cover   │  │ Squad Create   │  │                        │   ║   │          ║
║        │            ║  └───────┬────────┘  └───────┬────────┘  └────────────────────────┘   ║   │          ║
║        │            ╚══════════╪══════════════════╪═════════════════════════════════════════╝   │          ║
║        │                       │                  │                                             │          ║
║        ▼                       ▼                  ▼                                             │          ║
║   ┌──────────┐      ╔════════════════════════════════════════════════════════════════════════╗  │          ║
║   │ Allocate │      ║  L4: TASK ALLOCATION & COORDINATION                                    ║  │          ║
║   │ Command  │─────▶║  ┌────────────────┐  ┌────────────────┐  ┌────────────────────────┐   ║  │          ║
║   │          │      ║  │ Task Allocator │  │ Formation Ctrl │  │ Squad Coordinator      │   ║  │          ║
║   └──────────┘      ║  │ AutoFillParams │  │ Line/Wedge/... │  │ Leader + Members       │   ║  │          ║
║                     ║  └───────┬────────┘  └───────┬────────┘  └───────────┬────────────┘   ║  │          ║
║        │            ╚══════════╪══════════════════╪════════════════════════╪════════════════╝  │          ║
║        │                       │                  │                        │                   │          ║
║        │                       └──────────────────┴────────────────────────┘                   │          ║
║        │                                          │                                            │          ║
║        │                                          │ SetTag("Command.Patrol")                   │          ║
║        │                                          ▼                                            │          ║
║        │            ╔════════════════════════════════════════════════════════════════════════╗ │          ║
║        │            ║  L3: HETEROGENEOUS AGENTS                                              ║ │          ║
║        │            ║  ┌──────────────────┐ ┌──────────────────┐ ┌──────────────────────┐   ║ │          ║
║        │            ║  │       UGV        │ │       UAV        │ │    Human Avatar      │   ║ │          ║
║        │            ║  │ ┌──────────────┐ │ │ ┌──────────────┐ │ │ ┌──────────────────┐ │   ║ │          ║
║   ┌──────────┐      ║  │ │  StateTree   │ │ │ │  StateTree   │ │ │ │    StateTree     │ │   ║ │          ║
║   │ State    │      ║  │ │ ┌──────────┐ │ │ │ │ ┌──────────┐ │ │ │ │  ┌──────────┐   │ │   ║ │          ║
║   │ Machine  │─────▶║  │ │ │Idle│Patrol│ │ │ │ │ │Idle│Patrol│ │ │ │ │  │Idle│Patrol│   │ │   ║ │          ║
║   │ Response │      ║  │ │ └──────────┘ │ │ │ │ └──────────┘ │ │ │ │  └──────────┘   │ │   ║ │          ║
║   └──────────┘      ║  │ │      ↓       │ │ │ │      ↓       │ │ │ │       ↓         │ │   ║ │          ║
║        │            ║  │ │  GAS Skills  │ │ │ │  GAS Skills  │ │ │ │   GAS Skills    │ │   ║ │          ║
║        │            ║  │ │ Navigate/... │ │ │ │ Navigate/... │ │ │ │  Navigate/...   │ │   ║ │          ║
║        │            ║  │ └──────────────┘ │ │ └──────────────┘ │ │ └──────────────────┘ │   ║ │          ║
║        │            ║  │ ─────────────────│ │ ─────────────────│ │ ─────────────────────│   ║ │          ║
║   ┌──────────┐      ║  │ CAPABILITY:      │ │ CAPABILITY:      │ │ CAPABILITY:          │   ║ │          ║
║   │ Query    │      ║  │ ✓Pat ✓Fol ✓Cov ✓Ch│ │ ✓Pat ✓Fol ✓Cov ✓Ch│ │ ✓Pat ✓Fol ✗Cov ✗Ch  │   ║ │          ║
║   │Interface │─────▶║  │ PatrolPath=...   │ │ PatrolPath=...   │ │ PatrolPath=...      │   ║ │          ║
║   └──────────┘      ║  │ Energy=100       │ │ Energy=100       │ │                      │   ║ │          ║
║        │            ║  └────────┬─────────┘ └────────┬─────────┘ └──────────┬───────────┘   ║ │          ║
║        │            ║           │ Sensor             │ Sensor               │ Sensor        ║ │          ║
║        │            ║           ▼                    ▼                      ▼               ║ │          ║
║        │            ║  ┌──────────────────────────────────────────────────────────────────┐ ║ │          ║
║   ┌──────────┐      ║  │  PERCEPTION: RGB Camera │ Depth │ LiDAR (per agent)              │ ║ │          ║
║   │ Sensor   │─────▶║  └──────────────────────────────────────────────────────────────────┘ ║─┘          ║
║   │ Capture  │      ╚═══════════════════════════════╤════════════════════════════════════════╝            ║
║   └──────────┘                                      │                                                     ║
║        │                                            │ Physical Interaction                                ║
║        ▼                                            ▼                                                     ║
║   ┌──────────┐      ╔════════════════════════════════════════════════════════════════════════╗           ║
║   │ Physical │      ║  L2: PHYSICS-BASED ENVIRONMENT                                         ║           ║
║   │ Interact │─────▶║  ┌────────────────┐  ┌────────────────┐  ┌────────────────────────┐   ║           ║
║   └──────────┘      ║  │   Collision    │  │   Navigation   │  │   Semantic Entities    │   ║           ║
║                     ║  │   Detection    │  │      Mesh      │  │ PatrolPath/ChargeStn/  │   ║           ║
║        │            ║  │   (Capsule)    │  │   (NavMesh)    │  │ CoverageArea/Pickup    │   ║           ║
║        │            ║  └────────────────┘  └────────────────┘  └────────────────────────┘   ║           ║
║        │            ╚═══════════════════════════════╤════════════════════════════════════════╝           ║
║        │                                            │                                                     ║
║        ▼                                            ▼                                                     ║
║   ┌──────────┐      ╔════════════════════════════════════════════════════════════════════════╗           ║
║   │ Render   │      ║  L1: SIMULATION CORE (Unreal Engine 5)                                 ║           ║
║   │ Physics  │─────▶║  ┌──────────────────────────────┐  ┌────────────────────────────────┐ ║           ║
║   └──────────┘      ║  │    Real-time Rendering       │  │       PhysX Engine             │ ║           ║
║                     ║  │    (Nanite, Lumen)           │  │    (Rigid Body, Collision)     │ ║           ║
║                     ║  └──────────────────────────────┘  └────────────────────────────────┘ ║           ║
║                     ╚════════════════════════════════════════════════════════════════════════╝           ║
║                                                                                                           ║
╚═══════════════════════════════════════════════════════════════════════════════════════════════════════════╝
```

### Layer Summary

| Layer | Function | Key Components | I/O |
|:-----:|:---------|:---------------|:----|
| **L5** | Human-Robot Interface | RTS Commands, Selection, Video Feedback | Cmd↓ Feedback↑ |
| **L4** | Task Allocation | Allocator, Formation, Squad | Tag↓ to Agent |
| **L3** | Heterogeneous Agents | Agent = StateTree + GAS + Capability + Sensor | Tag→State→Skill |
| **L2** | Environment | Collision, NavMesh, Semantic Entities | Physics |
| **L1** | Simulation Core | UE5 Rendering, PhysX | Ground Truth |

### Agent Internal Architecture (L3 Detail)

```
┌─────────────────────────────────────────────────────────────────┐
│                         AGENT (UGV/UAV/Human)                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  StateTree Component (Strategic Layer)                    │  │
│  │  ┌──────┐ ┌────────┐ ┌────────┐ ┌──────────┐ ┌────────┐  │  │
│  │  │ Idle │◀┼▶Patrol │◀┼▶Follow │◀┼▶Coverage │◀┼▶Charge │  │  │
│  │  └──────┘ └────────┘ └────────┘ └──────────┘ └────────┘  │  │
│  │                         │ activate                        │  │
│  │                         ▼                                 │  │
│  │  GAS Component (Tactical Layer)                           │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────────────┐    │  │
│  │  │ Navigation │ │   Motion   │ │ Collision Avoidance│    │  │
│  │  └────────────┘ └────────────┘ └────────────────────┘    │  │
│  └───────────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  Capability Interfaces (Parameters)                       │  │
│  │  IPatrollable: PatrolPath, ScanRadius                     │  │
│  │  IFollowable:  FollowTarget                               │  │
│  │  ICoverable:   CoverageArea                               │  │
│  │  IChargeable:  Energy, MaxEnergy                          │  │
│  └───────────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  Sensor Components: Camera, Depth, LiDAR                  │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### Capability Matrix

|        | IPatrollable | IFollowable | ICoverable | IChargeable |
|:------:|:------------:|:-----------:|:----------:|:-----------:|
| **UGV** | ✓ | ✓ | ✓ | ✓ |
| **UAV** | ✓ | ✓ | ✓ | ✓ |
| **Human** | ✓ | ✓ | ✗ | ✗ |

### Key Design Principles

| Principle | Description | Benefit |
|-----------|-------------|---------|
| **Hierarchical Decomposition** | Strategic (what) vs Tactical (how) separation | Modular behavior design, easier debugging |
| **Capability Abstraction** | Interface-based polymorphism for agent types | Same task code works for UGV/UAV/Human |
| **Configuration-Driven** | JSON-based agent spawning, no code changes | Rapid scenario prototyping |
| **Real-time Feedback** | TCP video streaming, sensor data export | Human-in-the-loop supervision |

---

<details>
<summary>📋 Implementation Details (Click to expand)</summary>

### 架构亮点 (Technical Innovations)

| 层级 | 创新点 | 技术优势 |
|------|--------|----------|
| **Human-Robot Interface** | JSON 配置驱动 + Action 动态发现 | 零代码扩展新 Agent 类型，运行时热配置 |
| **Task Allocation** | RTS 风格命令系统 + Subsystem 统一访问 | 类 StarCraft 操控体验，MA_SUBS 宏简化访问 |
| **Behavior Architecture** | State Tree + GAS 黄金搭档 | 高层策略与底层技能解耦，Gameplay Tags 桥接 |
| **Agent Abstraction** | Interface 多态 + 异构 Agent | 同一 Task 支持 RobotDog/Drone，参数存储在实体 |
| **Perception** | Component 模式 + TCP 实时流 | 传感器即插即用，支持远程监控 |
| **Environment** | 语义化环境实体 | PatrolPath/CoverageArea 等可视化编辑 |

</details>

---

## 1. 系统概述

MultiAgent-Unreal 是一个基于 Unreal Engine 5 的多智能体仿真框架，用于机器人、无人机、角色等异构智能体的协同仿真。

## 2. 核心架构：CARLA 风格 + State Tree + GAS

### 2.1 CARLA 风格设计 + Interface 架构

本项目采用 **CARLA 风格架构 + UE Interface 模式**，参数存储在实体上，通过 Interface 实现多态：

```
┌─────────────────────────────────────────────────────────────┐
│                    Interface 架构                            │
│                                                             │
│  ┌─────────────────┐            ┌─────────────────┐        │
│  │ RobotDog        │            │ Drone           │        │
│  │ implements:     │            │ implements:     │        │
│  │ - IMAPatrollable│            │ - IMAPatrollable│        │
│  │ - IMAFollowable │            │ - IMAFollowable │        │
│  │ - IMACoverable  │            │ - IMACoverable  │        │
│  │ - IMAChargeable │            │ - IMAChargeable │        │
│  └────────┬────────┘            └────────┬────────┘        │
│           │                              │                  │
│           └──────────┬───────────────────┘                  │
│                      ▼                                      │
│           ┌─────────────────────┐                          │
│           │  StateTree Tasks    │                          │
│           │  (使用 Interface)    │                          │
│           │  - MASTTask_Patrol  │                          │
│           │  - MASTTask_Follow  │                          │
│           │  - MASTTask_Coverage│                          │
│           │  - MASTTask_Charge  │                          │
│           └─────────────────────┘                          │
└─────────────────────────────────────────────────────────────┘
```

**核心原则：**
- 参数存储在 Agent 上（如 `ScanRadius`, `FollowTarget`）
- Task/Ability 通过 Interface 获取参数，不硬编码类型
- 同一参数可被多个 Task/Ability 复用
- 运行时可动态修改
- 新增 Agent 类型只需实现对应 Interface

**Agent Interface 定义 (MAAgentInterfaces.h):**

| Interface | 方法 | 用途 |
|-----------|------|------|
| `IMAPatrollable` | `GetPatrolPath()`, `SetPatrolPath()`, `GetScanRadius()` | 巡逻能力 |
| `IMAFollowable` | `GetFollowTarget()`, `SetFollowTarget()`, `ClearFollowTarget()` | 跟随能力 |
| `IMACoverable` | `GetCoverageArea()`, `SetCoverageArea()`, `GetScanRadius()` | 覆盖扫描能力 |
| `IMAChargeable` | `GetEnergy()`, `RestoreEnergy()`, `DrainEnergy()`, `HasEnergy()` | 充电能力 |

**Agent 共用参数：**

| 属性 | 类型 | 用途 |
|------|------|------|
| `ScanRadius` | float | Follow 距离、Coverage 间距、Patrol 到达判定 |
| `FollowTarget` | AMACharacter* | 跟随目标 |
| `CoverageArea` | AActor* | 覆盖区域 |
| `PatrolPath` | AMAPatrolPath* | 巡逻路径 |
| `Energy/MaxEnergy` | float | 能量系统 |

### 2.2 State Tree + GAS 黄金搭档

本项目采用 **State Tree + GAS** 黄金搭档架构：

```
State Tree (大脑 - 状态决策)          GAS (手脚 - 技能执行)
┌─────────────────────────┐         ┌─────────────────────────┐
│  Root                   │         │  Abilities              │
│  ├── Exploration State  │◄───────►│  ├── GA_Pickup          │
│  ├── PhotoMode State    │  Tags   │  ├── GA_Drop            │
│  └── Interaction State  │◄───────►│  ├── GA_Navigate        │
│      ├── Pickup         │         │  └── GA_Follow          │
│      └── Dialogue       │         │                         │
└─────────────────────────┘         └─────────────────────────┘
```

- **State Tree**: 高层策略决策，管理 Character 的主模式（探索/拍照/交互）
- **GAS**: 具体技能执行，处理动画、冷却、消耗等
- **Gameplay Tags**: 两者之间的通信桥梁

### 2.3 RTS 命令系统架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           RTS 命令系统架构                                    │
│                                                                             │
│  ┌─────────────────┐                                                        │
│  │ MAPlayerController │  ◄── 键盘/鼠标输入                                   │
│  │ (Input 层)        │                                                      │
│  │ - 框选/点选       │                                                      │
│  │ - 快捷键绑定      │                                                      │
│  └────────┬─────────┘                                                       │
│           │                                                                 │
│           │ MA_SUBS 宏访问                                                  │
│           ▼                                                                 │
│  ┌────────────────────────────────────────────────────────────────────┐    │
│  │  FMASubsystem (统一访问层)                                          │    │
│  │  ┌─────────────────────────────────────────────────────────────┐   │    │
│  │  │ MA_SUBS.AgentManager / CommandManager / SelectionManager... │   │    │
│  │  └─────────────────────────────────────────────────────────────┘   │    │
│  │                                                                     │    │
│  │                    Manager 层 (UWorldSubsystem)                     │    │
│  │                                                                     │    │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐    │    │
│  │  │ SelectionManager│  │ CommandManager  │  │  SquadManager   │    │    │
│  │  │ 选择管理        │  │ 命令分发        │  │  编队管理       │    │    │
│  │  │                 │  │                 │  │                 │    │    │
│  │  │ - SelectAgent() │  │ - SendCommand() │  │ - CreateSquad() │    │    │
│  │  │ - BoxSelect()   │  │ - AutoFillParams│  │ - CycleFormation│    │    │
│  │  │ - ControlGroups │  │ - Patrol/Follow │  │ - GetOrCreate() │    │    │
│  │  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘    │    │
│  │           │                    │                    │              │    │
│  └───────────┼────────────────────┼────────────────────┼──────────────┘    │
│              │                    │                    │                    │
│              │                    │ 设置 Command Tag   │ 管理                │
│              │                    ▼                    ▼                    │
│  ┌───────────┼────────────────────────────────────────────────────────┐    │
│  │           │              Entity 层                                  │    │
│  │           │                                                         │    │
│  │           │    ┌─────────────────┐      ┌─────────────────┐        │    │
│  │           │    │  AgentManager   │      │    UMASquad     │        │    │
│  │           │    │  Agent 生命周期  │      │  编队实体       │        │    │
│  │           │    │  - SpawnAgent() │      │  - Members[]    │        │    │
│  │           │    │  - GetAgents()  │      │  - Leader       │        │    │
│  │           │    └────────┬────────┘      │  - Formation()  │        │    │
│  │           │             │               └─────────────────┘        │    │
│  │           │             │ 管理                                      │    │
│  │           ▼             ▼                                           │    │
│  │  ┌──────────────────────────────────────────────────────────────┐  │    │
│  │  │                    AMACharacter (Agent)                       │  │    │
│  │  │                                                               │  │    │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │    │
│  │  │  │ StateTree   │  │    ASC      │  │  Sensors    │           │  │    │
│  │  │  │ 状态决策    │◄─┤ 技能执行    │  │ 传感器组件  │           │  │    │
│  │  │  │             │  │             │  │             │           │  │    │
│  │  │  │ 监听 Command│  │ GA_Patrol   │  │ Camera      │           │  │    │
│  │  │  │ Tag 变化    │  │ GA_Follow   │  │ Lidar (TBD) │           │  │    │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │    │
│  │  └──────────────────────────────────────────────────────────────┘  │    │
│  └────────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘
```

**命令数据流:**
```
用户按键 (G) → PlayerController → CommandManager::SendCommand(Patrol)
                                        │
                                        ├── AutoFillCommandParams() ← 自动查找 PatrolPath
                                        ├── GetControllableAgents() ← 获取 RobotDog + Drone
                                        └── ApplyCommand() → ASC->AddLooseGameplayTag("Command.Patrol")
                                                                    │
                                                                    ▼
                                                            StateTree 检测 Tag → MASTTask_Patrol 执行
```

**Manager 职责:**

| Manager | 职责 | 不负责 |
|---------|------|--------|
| **SelectionManager** | 框选、点选、编组 (Ctrl+1~9) | 命令执行 |
| **CommandManager** | 命令分发、参数自动填充 | Agent 生命周期 |
| **SquadManager** | Squad 创建/解散、编队切换 | 单个 Agent 控制 |
| **AgentManager** | Agent 生命周期、JSON 配置 | 命令分发 |
| **ViewportManager** | 视角切换、相机管理、Agent View Mode (Direct Control) | - |

### 2.4 FMASubsystem 统一访问层

类似 Python 的依赖注入容器，提供全局单点访问所有 Manager Subsystem：

```cpp
// 传统方式 (繁琐)
UMACommandManager* CommandManager = GetWorld()->GetSubsystem<UMACommandManager>();
if (CommandManager) CommandManager->SendCommand(EMACommand::Patrol);

// 使用 MA_SUBS 宏 (简洁)
MA_SUBS.CommandManager->SendCommand(EMACommand::Patrol);
MA_SUBS.AgentManager->GetAllAgents();
MA_SUBS.SelectionManager->GetSelectedAgents();
```

**FMASubsystem 结构:**
```cpp
struct FMASubsystem
{
    UMAAgentManager* AgentManager;
    UMACommandManager* CommandManager;
    UMASelectionManager* SelectionManager;
    UMASquadManager* SquadManager;
    UMAViewportManager* ViewportManager;
    UMAEmergencyManager* EmergencyManager;  // 突发事件管理

    static FMASubsystem Get(UWorld* World);
};

#define MA_SUBS FMASubsystem::Get(GetWorld())
```

## 3. 当前文件结构

```
MultiAgent-Unreal/
├── config/                          # 配置文件目录
│   └── agents.json                  # Agent 配置 (JSON 驱动创建)
│
└── unreal_project/Source/MultiAgent/
    ├── Core/                        # 核心框架
    │   ├── MATypes.h                # 公共类型定义 (EMAAgentType, EMAFormationType)
    │   ├── MASubsystem.h/cpp        # Subsystem 统一访问层 (MA_SUBS 宏)
    │   ├── MAAgentManager.h/cpp     # Agent 管理器 (JSON 配置 + Action 动态发现)
    │   ├── MACommandManager.h/cpp   # 命令管理器 (RTS 风格命令分发)
    │   ├── MASelectionManager.h/cpp # 选择管理器 (框选、编组)
    │   ├── MASquadManager.h/cpp     # 编队管理器
    │   ├── MAViewportManager.h/cpp  # 视角管理器
    │   ├── MAGameMode.h/cpp         # 游戏模式 (配置驱动)
    │   └── MAPlayerController.h/cpp # 玩家控制器 (Enhanced Input)
    │
    ├── Agent/                       # 智能体模块
    │   ├── Character/               # ACharacter 派生类
    │   │   ├── MACharacter.h/cpp    # 角色基类 (GAS + Sensor + Action 聚合)
    │   │   ├── MAHumanCharacter.h/cpp   # 人类角色
    │   │   ├── MARobotDogCharacter.h/cpp # 机器狗角色 (实现 4 个 Interface)
    │   │   ├── MADroneCharacter.h/cpp   # 无人机基类 (Abstract, 实现 4 个 Interface)
    │   │   ├── MADronePhantom4Character.h/cpp  # DJI Phantom 4 (小型)
    │   │   └── MADroneInspire2Character.h/cpp  # DJI Inspire 2 (大型, 边长3倍)
    │   │
    │   ├── Interface/               # Agent 能力接口
    │   │   └── MAAgentInterfaces.h  # IMAPatrollable, IMAFollowable, IMACoverable, IMAChargeable
    │   │
    │   ├── Component/               # 组件模块 (UE Component 模式)
    │   │   └── Sensor/              # 传感器组件
    │   │       ├── MASensorComponent.h/cpp       # 传感器基类 (Action 接口)
    │   │       └── MACameraSensorComponent.h/cpp # 摄像头组件 (6 个 Actions)
│   │
    │   ├── GAS/                     # GAS 模块
    │   │   ├── MAAbilitySystemComponent.h/cpp  # GAS 核心组件
    │   │   ├── MAGameplayTags.h/cpp # Gameplay Tags 定义
    │   │   ├── MAGameplayAbilityBase.h/cpp  # Ability 基类
    │   │   └── Ability/             # 具体技能
    │   │       ├── GA_Pickup.h/cpp  # 拾取技能
    │   │       ├── GA_Drop.h/cpp    # 放下技能
    │   │       ├── GA_Navigate.h/cpp # 导航技能
    │   │       ├── GA_Follow.h/cpp  # 追踪技能
    │   │       ├── GA_Search.h/cpp  # 搜索技能
    │   │       ├── GA_Observe.h/cpp # 观察技能
    │   │       ├── GA_Report.h/cpp  # 报告技能
    │   │       ├── GA_Charge.h/cpp  # 充电技能
    │   │       ├── GA_Formation.h/cpp # 编队技能
    │   │       └── GA_Avoid.h/cpp   # 避障技能
    │   │
    │   └── StateTree/               # State Tree 模块
    │       ├── MAStateTreeComponent.h/cpp   # State Tree 组件
    │       ├── Task/                # State Tree Task
    │       │   ├── MASTTask_ActivateAbility.h/cpp
    │       │   ├── MASTTask_Navigate.h/cpp
    │       │   ├── MASTTask_Patrol.h/cpp
    │       │   ├── MASTTask_Charge.h/cpp
    │       │   ├── MASTTask_Coverage.h/cpp
    │       │   └── MASTTask_Follow.h/cpp
    │       └── Condition/           # State Tree Condition
    │           ├── MASTCondition_HasCommand.h/cpp
    │           ├── MASTCondition_LowEnergy.h/cpp
    │           ├── MASTCondition_FullEnergy.h/cpp
    │           └── MASTCondition_HasPatrolPath.h/cpp
    │
    ├── Environment/                 # 环境 Actor
    │   ├── MAPickupItem.h/cpp       # 可拾取物品
    │   ├── MAChargingStation.h/cpp  # 充电站
    │   ├── MAPatrolPath.h/cpp       # 巡逻路径
    │   └── MACoverageArea.h/cpp     # 覆盖区域
    │
    ├── Input/                       # 输入系统
    │   ├── MAInputActions.h/cpp     # Input Actions 单例 (运行时创建)
    │   ├── MAInputConfig.h/cpp      # 输入配置数据资产
    │   ├── MAInputComponent.h/cpp   # 增强输入组件
    │   └── MAPlayerController.h/cpp # 玩家控制器
    │
    ├── MultiAgent.Build.cs          # 模块构建配置 (含 Json 模块)
    └── MultiAgent.h/cpp             # 模块定义
```

## 4. 类继承关系

```
ACharacter (UE) + IAbilitySystemInterface
    └── AMACharacter (角色基类，GAS + Sensor + Action 聚合)
            ├── AMAHumanCharacter (人类)
            ├── AMARobotDogCharacter (机器狗) + IMAPatrollable + IMAFollowable + IMACoverable + IMAChargeable
            └── AMADroneCharacter (无人机基类, Abstract) + IMAPatrollable + IMAFollowable + IMACoverable + IMAChargeable
                    ├── AMADronePhantom4Character (DJI Phantom 4, 小型)
                    └── AMADroneInspire2Character (DJI Inspire 2, 大型, 边长3倍)

USceneComponent (UE)
    └── UMASensorComponent (传感器基类，Action 接口)
            └── UMACameraSensorComponent (摄像头组件，6 个 Actions)

AActor (UE)
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
            ├── UGA_Search
            ├── UGA_Observe
            ├── UGA_Report
            ├── UGA_Charge
            ├── UGA_Formation
            └── UGA_Avoid

UWorldSubsystem (UE)
    └── UMAAgentManager (Agent 管理 + JSON 配置 + Action 路由)
```

## 5. JSON 配置驱动系统

### 5.1 配置文件结构

Agent 创建由 `config/agents.json` 驱动，无需修改 C++ 代码：

```json
{
  "version": "1.0",
  "spawn_settings": {
    "default_spacing": 150.0,
    "auto_position_enabled": true
  },
  "agents": [
    {
      "id": "human_01",
      "type": "Human",
      "class": "/Game/Blueprints/BP_MAHumanCharacter.BP_MAHumanCharacter_C",
      "position": "auto",
      "rotation": { "pitch": 0, "yaw": 0, "roll": 0 },
      "sensors": [
        {
          "type": "Camera",
          "position": { "x": -200, "y": 0, "z": 100 },
          "rotation": { "pitch": -10, "yaw": 0, "roll": 0 },
          "params": { "resolution": { "x": 640, "y": 480 }, "fov": 90 }
        }
      ]
    },
    {
      "id": "dog_01",
      "type": "RobotDog",
      "class": "/Game/Blueprints/BP_MARobotDogCharacter.BP_MARobotDogCharacter_C",
      "position": { "x": 500, "y": 200, "z": 100 }
    }
  ]
}
```

### 5.2 配置字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | string | Agent 唯一标识 (FString) |
| `type` | string | Agent 类型: Human, RobotDog, Drone, Camera |
| `class` | string | Blueprint 类路径 |
| `position` | object/"auto" | 生成位置，"auto" 自动计算 |
| `rotation` | object | 生成旋转 (pitch/yaw/roll) |
| `sensors` | array | 传感器配置列表 |

### 5.3 加载流程

```
MAGameMode::BeginPlay()
    │
    ▼
MAAgentManager::LoadAndSpawnFromConfig(path)
    │
    ├── 解析 JSON 配置
    ├── 遍历 agents[]
    │   ├── 加载 Blueprint 类
    │   ├── 计算生成位置 (auto 或指定)
    │   ├── SpawnAgent()
    │   └── 添加 Sensors
    └── 返回成功/失败
```

## 6. Manager 子系统设计

### 6.1 全局 Manager vs Character 组件

```
┌─────────────────────────────────────────────────────────────┐
│                    全局 Manager 层                           │
│                  UWorldSubsystem                             │
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │AgentManager │  │RelationMgr  │  │ MapManager  │         │
│  │ JSON 配置   │  │ 管理关系    │  │ 管理地图    │         │
│  │ Action 路由 │  │ 之间的关系  │  │ 导航感知    │         │
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
│  ├───────────┤         ├───────────┤         ├───────────┤ │
│  │ Sensors   │         │ Sensors   │         │ Sensors   │ │
│  │ 传感器组件│         │ 传感器组件│         │ 传感器组件│ │
│  └───────────┘         └───────────┘         └───────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 6.2 Manager 列表

| Manager | 层级 | 功能介绍 | 状态 |
|---------|------|---------|------|
| **MAAgentManager** | 全局 | JSON 配置加载 + Agent 生命周期 + Action 路由 + 编队管理 | ✅ 已实现 |
| **MAEmergencyManager** | 全局 | 突发事件触发/结束 + 状态管理 + SourceAgent 追踪 | ✅ 已实现 |
| **RelationManager** | 全局 | 实体关系管理 | ❌ 待开发 |
| **MapManager** | 全局 | 地图感知 | ❌ 待开发 |
| **StateTree** | Character级 | 状态决策 | ✅ 已实现 |
| **ASC** | Character级 | 技能执行 | ✅ 已实现 |
| **Sensors** | Character级 | 传感器组件 (Action 接口) | ✅ 已实现 |
| **ViewportManager** | 全局 | 视角切换 + Agent View Mode (Direct Control) | ✅ 已实现 |

### 6.3 ViewportManager 与 Direct Control

ViewportManager 负责管理相机视角切换和 Agent View Mode (Direct Control 模式)。

**核心功能:**
- 视角切换：Tab 键在不同 Agent 相机之间切换
- Agent View Mode：进入 Agent 视角后可直接控制该 Agent
- Direct Control：WASD 移动、鼠标/方向键旋转视角

**Agent View Mode 数据流:**
```
用户按 Tab → ViewportManager::SwitchToNextCamera()
                    │
                    ├── EnterAgentViewMode(Agent)
                    │   ├── bIsInAgentViewMode = true
                    │   ├── Agent->SetDirectControl(true)
                    │   └── HUD->ShowDirectControlIndicator(Agent)
                    │
用户按 WASD → PlayerController::OnMoveInput()
                    │
                    └── ViewportManager->ApplyMovementInput()
                        └── Agent->ApplyDirectMovement(Direction)

用户按 0 → ViewportManager::ReturnToSpectator()
                    │
                    ├── ExitAgentViewMode()
                    │   ├── Agent->SetDirectControl(false)
                    │   └── HUD->HideDirectControlIndicator()
                    └── 移动 Spectator 到 (790, 1810, 502)
```

**ViewportManager 属性:**
| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| bIsInAgentViewMode | bool | false | 是否处于 Agent 视角模式 |
| ControlledAgent | TWeakObjectPtr | null | 当前控制的 Agent |
| LookSensitivityYaw | float | 14.0 | 水平旋转灵敏度 |
| LookSensitivityPitch | float | 14.0 | 垂直俯仰灵敏度 |
| MinCameraPitch | float | -60.0 | 最小俯仰角 |
| MaxCameraPitch | float | 30.0 | 最大俯仰角 |

**Direct Control 特性:**
- 进入 Direct Control 后，Agent 的 `bOrientRotationToMovement` 被禁用，防止后退时振荡
- RTS 命令 (Patrol, Follow, Coverage) 不会影响处于 Direct Control 的 Agent
- Drone 支持 Space/Ctrl 垂直移动
- 方向键灵敏度为鼠标的 0.125 倍，适合精细调整

### 6.4 Squad 系统

参考 Company of Heroes 的 Squad 系统，Squad 是一组 Agent 的组合，拥有自己的技能（编队、协同攻击等）。

```
┌─────────────────────────────────────────────────────────────┐
│                    Squad 系统架构                            │
│                                                             │
│  ┌─────────────────┐         ┌─────────────────────────┐   │
│  │ MASquadManager  │         │      UMASquad           │   │
│  │ (WorldSubsystem)│ 管理 ──►│  - SquadID/Name         │   │
│  │ - CreateSquad   │         │  - Leader               │   │
│  │ - DisbandSquad  │         │  - Members[]            │   │
│  │ - GetSquadBy... │         │  - FormationType        │   │
│  └─────────────────┘         │  - StartFormation()     │   │
│                              │  - StopFormation()      │   │
│                              │  - ExecuteSkill()       │   │
│                              └─────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

**Squad 管理 API:**
```cpp
// 创建 Squad
UMASquad* Squad = SquadManager->CreateSquad(Members, Leader, "MySquad");

// 解散 Squad
SquadManager->DisbandSquad(Squad);

// 查询
UMASquad* Squad = SquadManager->GetSquadByAgent(Agent);
```

**编队类型 (EMAFormationType):**
| 类型 | 说明 |
|------|------|
| None | 无编队 |
| Line | 横向一字排开 |
| Column | 纵队 |
| Wedge | V形楔形 |
| Diamond | 菱形 |
| Circle | 圆形 |

**编队参数 (UMASquad):**
| 参数 | 默认值 | 说明 |
|------|--------|------|
| FormationSpacing | 200 | 成员间距 |
| FormationUpdateInterval | 0.3s | 更新频率 |
| FormationStartMoveThreshold | 60 | 超过此距离开始移动 |
| FormationStopMoveThreshold | 30 | 小于此距离停止移动 |

## 7. Sensor Component 设计

### 7.1 Component 模式架构

```
UMASensorComponent (USceneComponent)
    └── UMACameraSensorComponent
        - 作为 Character 的组件
        - 生命周期由 Character 管理
        - 通过 Character API 操作
```

### 7.2 MACharacter Sensor 管理 API

```cpp
// 添加相机传感器
UMACameraSensorComponent* Camera = Character->AddCameraSensor(
    FVector(-200, 0, 100),      // 相对位置
    FRotator(-10, 0, 0)         // 相对旋转
);

// 获取相机传感器
UMACameraSensorComponent* Camera = Character->GetCameraSensor();

// 获取所有传感器
TArray<UMASensorComponent*> Sensors = Character->GetAllSensors();

// 移除传感器
Character->RemoveSensor(Sensor);
```

### 7.3 MACameraSensorComponent 功能

| 方法 | 按键 | 说明 |
|------|------|------|
| `TakePhoto(FilePath)` | L | 拍照保存到 Saved/Screenshots/ |
| `StartRecording(FPS)` | R | 开始录像（序列帧） |
| `StopRecording()` | R | 停止录像 |
| `StartTCPStream(Port, FPS)` | V | 启动 TCP 视频流 |
| `StopTCPStream()` | V | 停止 TCP 视频流 |

## 8. Action 动态发现机制

### 8.1 Action 命名规范

```
<Category>.<ActionName>

Agent.NavigateTo       # Agent 自身 Action
Camera.TakePhoto       # Camera Sensor Action
Lidar.StartScan        # 未来 Lidar Sensor Action
```

### 8.2 接口定义

```cpp
// MASensorComponent (基类)
virtual TArray<FString> GetAvailableActions() const;
virtual bool ExecuteAction(const FString& ActionName, const TMap<FString, FString>& Params);

// MACharacter (聚合层)
TArray<FString> GetAvailableActions() const;
bool ExecuteAction(const FString& ActionName, const TMap<FString, FString>& Params);

// MAAgentManager (管理层)
TArray<FString> GetAgentAvailableActions(AMACharacter* Agent) const;
bool ExecuteAgentAction(AMACharacter* Agent, const FString& ActionName, const TMap<FString, FString>& Params);
```

### 8.3 已实现的 Actions

**Agent Actions:** `Agent.NavigateTo`, `Agent.CancelNavigation`, `Agent.Pickup`, `Agent.Drop`, `Agent.FollowActor`, `Agent.StopFollowing`

**Camera Actions:** `Camera.TakePhoto`, `Camera.StartRecording`, `Camera.StopRecording`, `Camera.StartTCPStream`, `Camera.StopTCPStream`, `Camera.GetFrameAsJPEG`

## 9. GAS 模块

### 9.1 Gameplay Tags

```cpp
// State Tags
State.Exploration, State.PhotoMode, State.Interaction

// Ability Tags
Ability.Pickup, Ability.Drop, Ability.Navigate, Ability.Follow,
Ability.Search, Ability.Observe, Ability.Report, Ability.Charge,
Ability.Formation, Ability.Avoid

// Status Tags
Status.Patrolling, Status.Searching, Status.Charging, Status.InFormation, Status.LowEnergy
```

### 9.2 Ability 列表

| Ability | 功能 | 激活条件 |
|---------|------|---------|
| `GA_Pickup` | 拾取物品 | 附近有可拾取物品 |
| `GA_Drop` | 放下物品 | Status.Holding |
| `GA_Navigate` | 导航移动 | 有 AIController |
| `GA_Follow` | 追踪目标 | 有目标 Character |
| `GA_Charge` | 充电 | 在充电站范围内 |
| `GA_Formation` | 编队移动 | 有 Leader |

## 10. State Tree 模块

### 10.1 自定义 Task

| Task | 功能 | 使用的 Interface |
|------|------|------------------|
| `MASTTask_Patrol` | 循环巡逻路径点 | `IMAPatrollable`, `IMAChargeable` |
| `MASTTask_Navigate` | 导航到指定位置 | - |
| `MASTTask_Charge` | 自动充电 | `IMAChargeable` |
| `MASTTask_Coverage` | 区域覆盖扫描 | `IMACoverable` |
| `MASTTask_Follow` | 跟随目标 | `IMAFollowable` |

### 10.2 自定义 Condition

| Condition | 功能 |
|-----------|------|
| `MASTCondition_HasCommand` | 检查是否收到指定命令 |
| `MASTCondition_LowEnergy` | 检查电量 < 阈值 |
| `MASTCondition_FullEnergy` | 检查电量 >= 阈值 |
| `MASTCondition_HasPatrolPath` | 检查是否有可用巡逻路径 |

## 11. 碰撞系统

### 11.1 Agent 间碰撞策略

项目统一使用 **CapsuleComponent** 实现 Agent 间碰撞：

```
┌─────────────────────────────────────────────────────────────┐
│                    碰撞系统架构                              │
│                                                             │
│  CapsuleComponent (ACharacter 默认)                         │
│  ├── 用途: 地面检测、移动系统、Agent 间碰撞                  │
│  ├── 对 Pawn 通道: ECR_Block (阻挡)                         │
│  └── 统一用于所有 Agent 的碰撞检测                          │
│                                                             │
│  SkeletalMesh                                               │
│  ├── 用途: 仅渲染                                           │
│  └── CollisionEnabled: NoCollision                          │
└─────────────────────────────────────────────────────────────┘
```

### 11.2 设计优势

| 方面 | 说明 |
|------|------|
| 简单 | 统一使用 CapsuleComponent，无需配置 Physics Asset |
| 可靠 | 不依赖模型配置，始终生效 |
| 性能 | 单一简单形状，计算快速 |

### 11.3 配置方式

碰撞在 `MACharacter` 基类中统一配置：

```cpp
// MACharacter.cpp - 构造函数中配置碰撞响应
GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

// MACharacter.cpp - BeginPlay 中自动计算 Capsule 大小
void AMACharacter::AutoFitCapsuleToMesh()
{
    FBoxSphereBounds Bounds = GetMesh()->Bounds;
    float Radius = FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y);
    float HalfHeight = Bounds.BoxExtent.Z;
    GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
}
```

子类无需手动设置 Capsule 大小，基类会根据 SkeletalMesh 边界自动计算。

### 11.4 Drone 移动碰撞检测

Drone 使用 `SetActorLocation()` 移动时启用 Sweep 检测：

```cpp
// MADroneCharacter::UpdateFlight()
FHitResult SweepHit;
SetActorLocation(NewLocation, true, &SweepHit);  // true = sweep
if (SweepHit.bBlockingHit)
{
    Hover();  // 碰撞时悬停
}
```

## 12. 输入系统 (Enhanced Input)

按键说明详见 [KEYBINDINGS.md](KEYBINDINGS.md)

## 13. UI 系统架构

### 13.1 UI 集成设计

本项目集成了 UIRef 项目的 UI 系统，实现了 TopDown 视角下的指令输入界面：

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           UI 系统架构                                        │
│                                                                             │
│  ┌─────────────────┐                                                        │
│  │ MAPlayerController │  ◄── Z 键切换 UI                                    │
│  │ (Input 层)        │                                                      │
│  │ - OnToggleMainUI  │                                                      │
│  └────────┬─────────┘                                                       │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MAHUD (继承 MASelectionHUD)                       │   │
│  │  ┌─────────────────┐  ┌─────────────────┐                           │   │
│  │  │ SimpleMainWidget│  │ SemanticMapWidget│                           │   │
│  │  │ (纯 C++ UI)    │  │ (后续阶段)      │                           │   │
│  │  └─────────────────┘  └─────────────────┘                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MACommSubsystem                                   │   │
│  │  - SendNaturalLanguageCommand()                                      │   │
│  │  - GenerateMockPlanResponse()                                        │   │
│  │  - OnPlannerResponse 委托                                            │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 13.2 UI 组件说明

| 组件 | 功能 | 实现方式 |
|------|------|----------|
| **MASimpleMainWidget** | 主界面 UI (输入框 + 结果显示) | 纯 C++ 动态创建 |
| **MAEmergencyWidget** | 突发事件详情界面 (相机画面 + 操作按钮) | 纯 C++ 动态创建 |
| **MACommSubsystem** | 通信子系统，处理指令和响应 | GameInstanceSubsystem |
| **MAHUD** | HUD 管理器，继承选择框绘制功能 | 继承自 MASelectionHUD |
| **MAGameInstance** | 全局配置管理 | 存储服务器地址等配置 |

### 13.4 突发事件系统

突发事件系统用于处理 Agent 在执行任务过程中发现的异常情况：

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        突发事件系统架构                                       │
│                                                                             │
│  触发方式:                                                                   │
│  ├── 键盘模拟: "-" 键 → ToggleEvent() → 默认使用 0 号机器狗                  │
│  └── Agent 自动: Agent → TriggerEventFromAgent(this) → 使用报告的 Agent      │
│                                                                             │
│  ┌─────────────────┐                                                        │
│  │ EmergencyManager│  ◄── 事件状态管理                                       │
│  │ (WorldSubsystem)│                                                        │
│  │ - SourceAgent   │  ← 触发事件的 Agent (任意 Agent)                        │
│  │ - bIsEventActive│  ← 事件激活状态                                         │
│  └────────┬────────┘                                                        │
│           │ OnEmergencyStateChanged 委托                                     │
│           ▼                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MAHUD                                             │   │
│  │  ┌─────────────────┐  ┌─────────────────┐                           │   │
│  │  │ EmergencyIndicator│ │ EmergencyWidget │                           │   │
│  │  │ (红色"突发事件") │ │ (详情界面)      │                           │   │
│  │  │ DrawHUD() 绘制   │ │ - 相机画面      │                           │   │
│  │  └─────────────────┘  │ - 操作按钮      │                           │   │
│  │                       │ - 输入框        │                           │   │
│  │                       └─────────────────┘                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

**API 设计 (解耦):**
- `EmergencyWidget.SetCameraSource(Camera)` - 只接收相机，不关心 Agent
- `EmergencyManager.GetSourceAgent()` - 返回触发事件的 Agent
- 视频流与 Agent 编号完全解耦，支持任意 Agent 触发事件

### 13.3 数据流程

```
用户按 Z 键 → PlayerController::OnToggleMainUI() → MAHUD::ToggleMainUI()
                                                        │
                                                        ▼
用户输入指令 → SimpleMainWidget::SubmitCommand() → OnCommandSubmitted 委托
                                                        │
                                                        ▼
HUD::OnSimpleCommandSubmitted() → CommSubsystem::SendNaturalLanguageCommand()
                                                        │
                                                        ▼
CommSubsystem::GenerateMockPlanResponse() → OnPlannerResponse 委托
                                                        │
                                                        ▼
HUD::OnPlannerResponse() → SimpleMainWidget::SetResultText() → 显示结果
```

### 13.4 UI 特性

- **Z 键切换**: 显示/隐藏主界面
- **自动聚焦**: UI 显示时自动聚焦到输入框
- **回车提交**: 输入框支持回车键提交指令
- **模拟响应**: 开发阶段使用模拟数据测试
- **兼容 RTS**: UI 显示时仍支持 RTS 命令快捷键
