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

### 2.1 Capability Component 架构 (2025-12 重构)

本项目采用 **Capability Component 模式**，类似 ROS 的 Parameter Server，但参数存储在 Agent 本地的 Component 中：

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Capability Component 架构                                 │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    Capability Components (复用)                      │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐     │   │
│  │  │ EnergyComponent │  │ PatrolComponent │  │ FollowComponent │     │   │
│  │  │ : IMAChargeable │  │ : IMAPatrollable│  │ : IMAFollowable │     │   │
│  │  │ Energy, MaxEnergy│  │ PatrolPath     │  │ FollowTarget    │     │   │
│  │  │ DrainRate       │  │ ScanRadius     │  │                 │     │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘     │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                      │
│                                      │ 组合使用                              │
│                                      ▼                                      │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  RobotDog / Drone (只需添加 Component，无重复代码)                    │   │
│  │  - EnergyComponent                                                   │   │
│  │  - PatrolComponent                                                   │   │
│  │  - FollowComponent                                                   │   │
│  │  - CoverageComponent                                                 │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                      │
│                                      │ 通过 Interface 访问                   │
│                                      ▼                                      │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  StateTree Tasks / GAS Abilities                                     │   │
│  │  GetCapabilityInterface<IMAChargeable>(Actor)->GetEnergy()          │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

**核心原则：**
- 能力参数封装在 Component 中（如 `UMAEnergyComponent`, `UMAPatrolComponent`）
- Component 实现对应的 Interface（如 `IMAChargeable`, `IMAPatrollable`）
- Task/Ability 通过 Interface 获取参数，不依赖具体 Character 类型
- 新增 Agent 类型只需组合 Component，零代码重复
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
    │   ├── MASceneGraphManager.h/cpp # 场景图管理器 (语义标注 + JSON 持久化)
    │   ├── MAEditModeManager.h/cpp  # Edit 模式管理器 (临时场景图 + POI/Goal/Zone)
    │   ├── MAGeometryUtils.h/cpp    # 几何计算工具类 (凸包、最近邻排序)
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
    │   ├── MACoverageArea.h/cpp     # 覆盖区域
    │   ├── MAPointOfInterest.h/cpp  # POI 标记点 (Edit Mode)
    │   ├── MAGoalActor.h/cpp        # Goal 可视化 Actor (Edit Mode)
    │   └── MAZoneActor.h/cpp        # Zone 可视化 Actor (Edit Mode)
    │
    ├── Input/                       # 输入系统
    │   ├── MAInputActions.h/cpp     # Input Actions 单例 (运行时创建)
    │   ├── MAInputConfig.h/cpp      # 输入配置数据资产
    │   ├── MAInputComponent.h/cpp   # 增强输入组件
    │   └── MAPlayerController.h/cpp # 玩家控制器
    │
    ├── UI/                          # UI 系统
    │   ├── MAHUD.h/cpp              # HUD 管理器 (继承 MASelectionHUD)
    │   ├── MASimpleMainWidget.h/cpp # 主界面 Widget (输入框 + 结果显示)
    │   ├── MAEmergencyWidget.h/cpp  # 突发事件详情界面
    │   ├── MAModifyWidget.h/cpp     # Modify 模式修改面板 (多选 + 标签编辑)
    │   ├── MAModifyTypes.h          # Modify 模式类型定义 (FMAAnnotationInput, EMAShapeType)
    │   ├── MAEditWidget.h/cpp       # Edit 模式编辑面板 (JSON 编辑 + Goal/Zone 创建)
    │   ├── MASceneListWidget.h/cpp  # Edit 模式左侧列表面板 (Goal/Zone 列表)
    │   ├── MATaskPlannerWidget.h/cpp # 任务规划工作台主容器
    │   ├── MADAGCanvasWidget.h/cpp  # DAG 画布 (拓扑排序布局)
    │   ├── MATaskNodeWidget.h/cpp   # 任务节点 Widget
    │   ├── MANodePaletteWidget.h/cpp # 节点工具栏
    │   ├── MAContextMenuWidget.h/cpp # 上下文菜单
    │   ├── MATaskGraphModel.h/cpp   # 任务图数据模型
    │   └── MATaskGraphTypes.h       # 任务图类型定义
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

UGameInstanceSubsystem (UE)
    └── UMASceneGraphManager (场景图管理 + 语义标注 + JSON 持久化)
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
| **MASceneGraphManager** | 全局 | 场景图管理 + 语义标注 + JSON 持久化 + GUID 反向查询 | ✅ 已实现 |
| **MAEditModeManager** | 全局 | Edit 模式管理 + 临时场景图 + POI/Goal/Zone 管理 | ✅ 已实现 |
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
| **MAModifyWidget** | Modify 模式修改面板 (Actor 标签编辑) | 纯 C++ 动态创建 |
| **MATaskPlannerWidget** | 任务规划工作台 (DAG 编辑器 + 指令输入) | 纯 C++ 动态创建 |
| **MADAGCanvasWidget** | DAG 画布 (拓扑排序布局 + 节点/边渲染) | 纯 C++ 动态创建 |
| **MATaskNodeWidget** | 任务节点 Widget (显示任务信息) | 纯 C++ 动态创建 |
| **MANodePaletteWidget** | 节点工具栏 (可拖拽节点模板) | 纯 C++ 动态创建 |
| **MATaskGraphModel** | 任务图数据模型 (JSON 序列化) | UObject |
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

### 13.5 Modify 模式系统

Modify 模式是第三种鼠标模式，用于查看和编辑场景中 Actor 的语义标签信息，支持单选和多选两种模式：

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Modify 模式系统架构                                    │
│                                                                             │
│  模式切换:                                                                   │
│  M 键 → Select → Deployment → Modify → Select (循环)                        │
│  (如果 Deployment 背包为空则跳过)                                            │
│                                                                             │
│  ┌─────────────────┐                                                        │
│  │ MAPlayerController│  ◄── 模式状态管理                                     │
│  │ (Input 层)        │                                                      │
│  │ - CurrentMouseMode│  ← EMAMouseMode::Modify                              │
│  │ - HighlightedActors│ ← 当前高亮的 Actor 数组 (支持多选)                   │
│  └────────┬─────────┘                                                       │
│           │ OnModifyActorsSelected 委托                                      │
│           ▼                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MAHUD                                             │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐      │   │
│  │  │ MASelectionHUD  │  │ MAModifyWidget  │  │ SceneLabels     │      │   │
│  │  │ DrawMouseMode() │  │ (修改面板)      │  │ (绿色文本标签)  │      │   │
│  │  │ 橙色 "Modify"   │  │ - 多行文本框    │  │ DrawDebugString │      │   │
│  │  └─────────────────┘  │ - 确认按钮      │  └─────────────────┘      │   │
│  │                       │ - JSON 预览     │                           │   │
│  │                       └─────────────────┘                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MASceneGraphManager                               │   │
│  │  - ParseLabelInput()      解析 "id:值,type:值,shape:值" 格式         │   │
│  │  - AddSceneNode()         添加节点到 JSON 文件                        │   │
│  │  - GetAllNodes()          获取所有节点用于可视化                       │   │
│  │  - FindNodesByGuid()      通过 GUID 反向查询所属节点                   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

**选择模式:**

| 操作 | 功能 |
|------|------|
| 左键点击 Actor | 单选模式，清除之前选择，选中当前 Actor |
| Shift+左键点击 | 多选模式，添加/移除 Actor 到选择集 (toggle) |
| 左键点击空白区域 | 清除所有选择 |

**输入格式:**

| 模式 | 输入格式 | 示例 |
|------|----------|------|
| 单选 (Point) | `id:值,type:值` | `id:building_01,type:building` |
| 多选 (Polygon) | `id:值,type:值,shape:polygon` | `id:area_01,type:building,shape:polygon` |
| 多选 (LineString) | `id:值,type:值,shape:linestring` | `id:road_01,type:road,shape:linestring` |

**JSON 输出格式:**

```json
// Point 类型 (单选)
{
  "id": "building_01",
  "guid": "A1B2C3D4-E5F6-7890-ABCD-EF1234567890",
  "properties": { "type": "building", "label": "Building-1" },
  "shape": { "type": "point", "center": [100, 200, 0] }
}

// Polygon 类型 (多选建筑群)
{
  "id": "area_01",
  "count": 3,
  "Guid": ["GUID1", "GUID2", "GUID3"],
  "properties": { "type": "building", "label": "Building-1" },
  "shape": { "type": "polygon", "vertices": [[x1,y1,z1], [x2,y2,z2], ...] }
}

// LineString 类型 (多选道路片段)
{
  "id": "road_01",
  "count": 5,
  "Guid": ["GUID1", "GUID2", "GUID3", "GUID4", "GUID5"],
  "properties": { "type": "road", "label": "Road-1" },
  "shape": { "type": "linestring", "points": [[x1,y1,z1], [x2,y2,z2], ...] }
}
```

**几何计算 (MAGeometryUtils):**

| 方法 | 功能 |
|------|------|
| `ComputeConvexHull2D()` | Graham Scan 算法计算凸包，返回逆时针顶点 |
| `CollectBoundingBoxCorners()` | 收集所有 Actor 边界框角点 |
| `SortByNearestNeighbor()` | 最近邻算法排序点序列 (用于 LineString) |
| `CalculatePolygonCentroid()` | 计算多边形几何中心 |
| `CalculateLineStringCentroid()` | 计算线串几何中心 |

**场景标签可视化:**
- 进入 Modify 模式时，自动加载 `datasets/scene_graph_cyberworld.json`
- 在已标注 Actor 位置显示绿色文本标签 (GUID + Label)
- 使用 `DrawDebugString` 绘制，位置偏移 Z+100
- 退出 Modify 模式时自动清除可视化

**GUID 反向查询:**
- 点击 Actor 时，通过 `FindNodesByGuid()` 查询其所属的 Polygon/LineString 节点
- 如果 Actor 属于某个节点，在 JSON 预览区显示完整节点信息
- 支持 Actor 属于多个节点的情况

**MAModifyWidget 接口:**

| 方法 | 功能 |
|------|------|
| `SetSelectedActors(Actors)` | 设置选中的 Actor 数组，更新 HintText 显示数量 |
| `ClearSelection()` | 清除选中状态，显示提示文字 |
| `ParseAnnotationInput()` | 解析用户输入，返回 FMAAnnotationInput |
| `GenerateSceneGraphNode()` | 根据输入和选中 Actor 生成 JSON 节点 |
| `UpdateJsonPreview()` | 更新 JSON 预览，支持 GUID 反向查询 |

**Actor 高亮实现:**
- 使用 UE5 的 Custom Depth Stencil 实现轮廓高亮
- 支持同时高亮多个 Actor
- 退出 Modify 模式时自动清除所有高亮

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
- **M 键切换**: 循环切换鼠标模式 (Select → Deployment → Modify → Select)
- **自动聚焦**: UI 显示时自动聚焦到输入框
- **回车提交**: 输入框支持回车键提交指令
- **模拟响应**: 开发阶段使用模拟数据测试
- **兼容 RTS**: UI 显示时仍支持 RTS 命令快捷键
- **Modify 模式**: 点击 Actor 显示高亮和修改面板，支持标签编辑


## 14. 通信协议 (Communication Protocol)

### 14.1 概述

MACommSubsystem 实现了仿真端与规划器后端之间的完整 HTTP 双向通信协议。系统支持多种消息类型的发送和接收，并为未来的消息类型扩展预留接口。

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        通信协议架构                                          │
│                                                                             │
│  ┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐       │
│  │ MASimpleMainWidget│    │ MAEmergencyWidget│    │  Task System    │       │
│  │ (UI 输入)        │    │ (按钮事件)       │    │ (任务反馈)      │       │
│  └────────┬─────────┘    └────────┬─────────┘    └────────┬────────┘       │
│           │                       │                       │                 │
│           │ UI_Input_Message      │ Button_Event_Message  │ Task_Feedback   │
│           └───────────────────────┼───────────────────────┘                 │
│                                   ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MACommSubsystem                                   │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐      │   │
│  │  │ Message Factory │  │ HTTP Client     │  │ Poll Timer      │      │   │
│  │  │ 创建消息信封    │  │ POST/GET 请求   │  │ 定期轮询        │      │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘      │   │
│  │                                                                      │   │
│  │  ┌─────────────────┐  ┌─────────────────┐                           │   │
│  │  │ JSON Serializer │  │ Response Parser │                           │   │
│  │  │ 序列化/反序列化 │  │ 解析 DAG/Graph  │                           │   │
│  │  └─────────────────┘  └─────────────────┘                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                   │                                         │
│                                   │ HTTP                                    │
│                                   ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    Planner Backend                                   │   │
│  │                    http://localhost:8080                             │   │
│  │  POST /api/sim/message  - 接收仿真端消息                             │   │
│  │  GET  /api/sim/poll     - 返回待处理消息                             │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 14.2 消息类型

#### 出站消息 (仿真端 → 规划器)

| 消息类型 | 枚举值 | 说明 |
|----------|--------|------|
| `UIInput` | ui_input | UI 输入框提交的文本数据 |
| `ButtonEvent` | button_event | 按钮点击事件 |
| `TaskFeedback` | task_feedback | 任务执行结果反馈 |

#### 入站消息 (规划器 → 仿真端)

| 消息类型 | 枚举值 | 说明 |
|----------|--------|------|
| `TaskPlanDAG` | task_plan_dag | 任务规划有向无环图 |

> **注意：** 世界模型（场景图）现在存储在仿真端本地，由 `MASceneGraphManager` 管理，不再通过轮询从后端获取。仿真端通过 `/api/sim/scene_change` 端点向后端同步场景变化。

### 14.3 消息格式

所有消息使用统一的 Message_Envelope 包装：

```json
{
    "message_type": "ui_input",
    "timestamp": 1702656000000,
    "message_id": "550e8400-e29b-41d4-a716-446655440000",
    "payload": {
        // 消息类型特定的数据
    }
}
```

#### UI 输入消息 (ui_input)

```json
{
    "message_type": "ui_input",
    "timestamp": 1702656000000,
    "message_id": "...",
    "payload": {
        "input_source_id": "SimpleMainWidget_InputBox",
        "input_content": "让机器人去巡逻"
    }
}
```

#### 按钮事件消息 (button_event)

```json
{
    "message_type": "button_event",
    "timestamp": 1702656000000,
    "message_id": "...",
    "payload": {
        "widget_name": "EmergencyWidget",
        "button_id": "btn_confirm",
        "button_text": "确认处理"
    }
}
```

#### 任务反馈消息 (task_feedback)

```json
{
    "message_type": "task_feedback",
    "timestamp": 1702656000000,
    "message_id": "...",
    "payload": {
        "task_id": "patrol_001",
        "status": "success",
        "duration_seconds": 120.5,
        "energy_consumed": 15.3,
        "error_message": ""
    }
}
```

#### 任务规划 DAG (task_plan_dag)

```json
{
    "message_type": "task_plan_dag",
    "timestamp": 1702656000000,
    "message_id": "...",
    "payload": {
        "nodes": [
            {
                "node_id": "node_1",
                "task_type": "navigate",
                "parameters": {"target_x": "100", "target_y": "200"},
                "dependencies": []
            },
            {
                "node_id": "node_2",
                "task_type": "patrol",
                "parameters": {"path_id": "path_01"},
                "dependencies": ["node_1"]
            }
        ],
        "edges": [
            {"from_node_id": "node_1", "to_node_id": "node_2"}
        ]
    }
}
```

### 14.4 API 使用方法

#### 发送消息

```cpp
// 获取 CommSubsystem
UMACommSubsystem* CommSubsystem = GetGameInstance()->GetSubsystem<UMACommSubsystem>();

// 发送 UI 输入消息
CommSubsystem->SendUIInputMessage(TEXT("MyWidget_InputBox"), TEXT("用户输入内容"));

// 发送按钮事件消息
CommSubsystem->SendButtonEventMessage(TEXT("MyWidget"), TEXT("btn_submit"), TEXT("提交"));

// 发送任务反馈消息
FMATaskFeedbackMessage Feedback;
Feedback.TaskId = TEXT("task_001");
Feedback.Status = TEXT("success");
Feedback.DurationSeconds = 60.0f;
Feedback.EnergyConsumed = 10.0f;
CommSubsystem->SendTaskFeedbackMessage(Feedback);
```

#### 接收消息 (委托绑定)

```cpp
// 绑定任务规划 DAG 接收委托
CommSubsystem->OnTaskPlanReceived.AddDynamic(this, &UMyClass::OnTaskPlanReceived);

void UMyClass::OnTaskPlanReceived(const FMATaskPlanDAG& TaskPlan)
{
    // 处理任务规划 DAG
    for (const FMATaskPlanNode& Node : TaskPlan.Nodes)
    {
        UE_LOG(LogTemp, Log, TEXT("Task Node: %s, Type: %s"), *Node.NodeId, *Node.TaskType);
    }
}

// 注意: 世界模型（场景图）现在由 MASceneGraphManager 本地管理
// OnWorldModelReceived 委托已废弃，不再被触发
```

#### 向后兼容

旧的 `SendNaturalLanguageCommand` API 仍然可用，内部会转换为 UI 输入消息：

```cpp
// 旧 API (仍然支持)
CommSubsystem->SendNaturalLanguageCommand(TEXT("让机器人去巡逻"));

// 等价于
CommSubsystem->SendUIInputMessage(TEXT("legacy_command"), TEXT("让机器人去巡逻"));
```

### 14.5 配置选项

通过 `config/SimConfig.json` 配置通信参数：

```json
{
    "PlannerServerURL": "http://localhost:8080",
    "bUseMockData": true,
    "bEnablePolling": true,
    "PollIntervalSeconds": 1.0
}
```

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `PlannerServerURL` | string | `http://localhost:8080` | 规划器服务器地址 |
| `bUseMockData` | bool | `true` | 是否使用模拟数据 (开发测试用) |
| `bEnablePolling` | bool | `true` | 是否启用轮询 |
| `PollIntervalSeconds` | float | `1.0` | 轮询间隔 (秒) |

### 14.6 错误处理

| 错误类型 | 处理策略 |
|---------|---------|
| 连接失败 | 重试 3 次，指数退避 (1s, 2s, 4s) |
| 超时 (>10s) | 取消请求，记录日志 |
| 4xx 错误 | 不重试，记录错误日志 |
| 5xx 错误 | 重试 3 次 |
| JSON 解析失败 | 记录原始响应，广播错误 |

### 14.7 Mock 模式

当 `bUseMockData = true` 时：
- 不发送实际 HTTP 请求
- 生成模拟响应数据
- 用于开发和测试
- 轮询功能自动禁用

### 14.8 UI 集成指南

#### 已集成的 Widget

| Widget | 按钮事件 | 输入消息 |
|--------|----------|----------|
| **MASimpleMainWidget** | `btn_send` (发送) | `SimpleMainWidget_InputBox` |
| **MAEmergencyWidget** | `btn_expand_search` (扩大搜索范围)<br>`btn_ignore_return` (忽略并返回)<br>`btn_switch_firefight` (切换灭火任务)<br>`btn_send` (发送) | `EmergencyWidget_InputBox` |
| **MATaskPlannerWidget** | `btn_update_graph` (更新任务图)<br>`btn_send_command` (发送指令) | `TaskPlannerWidget_InputBox` |

#### 新增 Widget 集成步骤

为新 UI Widget 添加通信支持：

1. **添加头文件引用**
```cpp
#include "MACommSubsystem.h"
```

2. **在按钮点击处理函数中上报事件**
```cpp
if (UGameInstance* GameInstance = GetGameInstance())
{
    if (UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>())
    {
        CommSubsystem->SendButtonEventMessage(
            TEXT("YourWidgetName"),     // widget_name
            TEXT("btn_xxx"),            // button_id
            TEXT("按钮文字")            // button_text
        );
    }
}
```

3. **在提交输入时上报消息**
```cpp
CommSubsystem->SendUIInputMessage(
    TEXT("YourWidgetName_InputBox"),    // input_source_id
    InputContent                         // input_content
);
```

#### 验收测试日志

操作 EmergencyWidget 时应看到以下日志：

**点击操作按钮：**
```
LogMAEmergencyWidget: Action Button 1 clicked: 扩大搜索范围
LogMACommSubsystem: SendButtonEventMessage: Widget=EmergencyWidget, ButtonId=btn_expand_search, ButtonText=扩大搜索范围
```

**点击发送按钮：**
```
LogMAEmergencyWidget: Send button clicked
LogMACommSubsystem: SendButtonEventMessage: Widget=EmergencyWidget, ButtonId=btn_send, ButtonText=发送
LogMAEmergencyWidget: Submitting message: xxx
LogMACommSubsystem: SendUIInputMessage: SourceId=EmergencyWidget_InputBox, Content=xxx
```

## 15. 任务规划工作台 (Task Planner Workbench)

### 15.1 概述

任务规划工作台是一个可视化的 DAG (有向无环图) 编辑器，用于创建、编辑和管理任务规划图。支持从规划器后端接收任务 DAG 并可视化展示，也支持手动编辑和发送自然语言指令。

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        任务规划工作台架构                                     │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MATaskPlannerWidget (主容器)                      │   │
│  │  ┌─────────────────────┬───────────────────────────────────────────┐│   │
│  │  │   Left Panel        │         Right Panel                       ││   │
│  │  │  ┌───────────────┐  │  ┌─────────────────────────────────────┐ ││   │
│  │  │  │ Status Log    │  │  │     DAG Canvas                      │ ││   │
│  │  │  │ (只读日志)    │  │  │  ┌─────────┐  ┌─────────┐          │ ││   │
│  │  │  └───────────────┘  │  │  │ Node A  │──│ Node B  │          │ ││   │
│  │  │  ┌───────────────┐  │  │  └─────────┘  └─────────┘          │ ││   │
│  │  │  │ JSON Editor   │  │  │       │                             │ ││   │
│  │  │  │ (可编辑)      │  │  │       ▼                             │ ││   │
│  │  │  └───────────────┘  │  │  ┌─────────┐                        │ ││   │
│  │  │  ┌───────────────┐  │  │  │ Node C  │                        │ ││   │
│  │  │  │ Update Button │  │  │  └─────────┘                        │ ││   │
│  │  │  └───────────────┘  │  └─────────────────────────────────────┘ ││   │
│  │  │  ┌───────────────┐  │  ┌─────────────────────────────────────┐ ││   │
│  │  │  │ 指令输入框    │  │  │     Node Palette (工具栏)           │ ││   │
│  │  │  └───────────────┘  │  │  [Navigate] [Patrol] [Custom]       │ ││   │
│  │  │  ┌───────────────┐  │  └─────────────────────────────────────┘ ││   │
│  │  │  │ 发送指令按钮  │  │                                          ││   │
│  │  │  └───────────────┘  │                                          ││   │
│  │  └─────────────────────┴───────────────────────────────────────────┘│   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 15.2 核心组件

| 组件 | 功能 | 实现方式 |
|------|------|----------|
| **MATaskPlannerWidget** | 主容器，管理布局和子组件 | 纯 C++ 动态创建 |
| **MADAGCanvasWidget** | DAG 画布，渲染节点和边 | 支持平移、缩放、拖拽 |
| **MATaskNodeWidget** | 任务节点 Widget | 显示任务类型、参数、状态 |
| **MANodePaletteWidget** | 节点工具栏 | 提供可拖拽的节点模板 |
| **MATaskGraphModel** | 数据模型 | 管理任务图数据和 JSON 序列化 |

### 15.3 拓扑排序布局算法

DAG 画布使用 **Kahn 算法** 进行拓扑排序，实现分层布局：

```cpp
// 算法流程
1. 构建邻接表和入度表
2. 将所有入度为 0 的节点加入队列 (第 0 层)
3. BFS 遍历，计算每个节点的层级
4. 按层级分组节点
5. 同层节点水平排列，层与层之间垂直排列
```

**布局参数：**
| 参数 | 默认值 | 说明 |
|------|--------|------|
| NodeSpacingX | 280.0f | 同层节点水平间距 |
| NodeSpacingY | 180.0f | 层与层之间垂直间距 |
| StartX | 80.0f | 起始 X 坐标 |
| StartY | 50.0f | 起始 Y 坐标 |

**布局效果：**
```
Layer 0:    [Node A]  [Node B]
                │         │
                ▼         ▼
Layer 1:    [Node C]  [Node D]
                │
                ▼
Layer 2:    [Node E]
```

### 15.4 用户指令输入

任务规划工作台集成了自然语言指令输入功能：

```
用户输入指令 → UserInputBox → OnSendCommandButtonClicked()
                                        │
                                        ▼
                              OnCommandSubmitted 委托
                                        │
                                        ▼
                              MAHUD::OnPlannerCommandSubmitted()
                                        │
                                        ▼
                              MACommSubsystem::SendUIInputMessage()
                                        │
                                        ▼
                              规划器后端 (HTTP POST)
```

**输入框特性：**
- 支持多行输入
- 提示文本："输入自然语言指令，例如：让机器人去巡逻..."
- 发送后自动清空输入框
- 发送的指令会记录到状态日志

### 15.5 画布交互

| 操作 | 功能 |
|------|------|
| 左键拖拽空白区域 | 平移画布 |
| 滚轮 | 缩放画布 (0.25x ~ 2.0x) |
| 左键点击节点 | 选中节点 |
| 左键拖拽节点 | 移动节点 |
| 从输出端口拖拽到输入端口 | 创建边 |
| Delete/Backspace | 删除选中的节点或边 |
| 双击节点 | 编辑节点 |
| 左键点击边 | 选中边 |

### 15.6 数据流程

```
规划器后端 → MACommSubsystem::OnTaskPlanReceived
                    │
                    ▼
            MAHUD::OnTaskPlanReceived()
                    │
                    ▼
            TaskPlannerWidget->LoadTaskGraph(Data)
                    │
                    ├── GraphModel->LoadFromData(Data)
                    ├── SyncJsonEditorFromModel()
                    └── DAGCanvas->RefreshFromModel()
                            │
                            ├── ClearAllNodes()
                            ├── CreateNode() for each node
                            ├── CreateEdge() for each edge
                            └── AutoLayoutNodes() (拓扑排序)
```

### 15.7 JSON 格式

任务图使用以下 JSON 格式：

```json
{
  "description": "巡逻任务规划",
  "nodes": [
    {
      "task_id": "node_1",
      "task_type": "navigate",
      "display_name": "导航到起点",
      "parameters": {
        "target_x": "100",
        "target_y": "200"
      },
      "status": "pending"
    },
    {
      "task_id": "node_2",
      "task_type": "patrol",
      "display_name": "执行巡逻",
      "parameters": {
        "path_id": "patrol_path_01"
      },
      "status": "pending"
    }
  ],
  "edges": [
    {
      "from_node_id": "node_1",
      "to_node_id": "node_2",
      "edge_type": "sequential"
    }
  ]
}
```

### 15.8 按键绑定

| 按键 | 功能 |
|------|------|
| **Z** | 切换任务规划工作台显示/隐藏 |

### 15.9 UI 集成

任务规划工作台已集成到 MAHUD 中：

| Widget | 按钮事件 | 输入消息 |
|--------|----------|----------|
| **MATaskPlannerWidget** | `btn_update_graph` (更新任务图)<br>`btn_send_command` (发送指令) | `TaskPlannerWidget_InputBox` |


## 16. 场景图管理系统 (Scene Graph Manager)

### 16.1 概述

场景图管理系统用于管理场景中 Actor 的语义标注数据，支持单选和多选标注，并将数据持久化到 JSON 文件。

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        场景图管理系统架构                                     │
│                                                                             │
│  ┌─────────────────┐                                                        │
│  │ MAPlayerController│  ◄── Modify 模式交互                                  │
│  │ - HighlightedActors│ ← 支持多选 (Shift+Click)                            │
│  │ - OnModifyActorsSelected│                                                │
│  └────────┬─────────┘                                                       │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MAModifyWidget                                    │   │
│  │  - ParseAnnotationInput()    解析用户输入                             │   │
│  │  - GenerateSceneGraphNode()  生成 JSON 节点                           │   │
│  │  - UpdateJsonPreview()       更新 JSON 预览                           │   │
│  └────────┬─────────────────────────────────────────────────────────────┘   │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MASceneGraphManager (UGameInstanceSubsystem)      │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐      │   │
│  │  │ ParseLabelInput │  │ AddSceneNode    │  │ GetAllNodes     │      │   │
│  │  │ 解析输入格式    │  │ 添加节点到 JSON │  │ 获取所有节点    │      │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘      │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐      │   │
│  │  │ GenerateLabel   │  │ IsIdExists      │  │ FindNodesByGuid │      │   │
│  │  │ 生成 Type-N 标签│  │ ID 去重检查     │  │ GUID 反向查询   │      │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘      │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                   │                                         │
│                                   ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    datasets/scene_graph_cyberworld.json              │   │
│  │  { "nodes": [ ... ] }                                                │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 16.2 数据结构

**FMASceneGraphNode 结构体:**

| 字段 | 类型 | 说明 |
|------|------|------|
| `Id` | FString | 用户指定的唯一标识符 |
| `Guid` | FString | Actor 的 GUID (单选时) |
| `GuidArray` | TArray<FString> | Actor GUID 数组 (多选时) |
| `Type` | FString | 语义类型 (building, road, etc.) |
| `Label` | FString | 自动生成的标签 (Type-N) |
| `Center` | FVector | 显示位置 (几何中心) |
| `ShapeType` | FString | 形状类型 (point, polygon, linestring) |
| `RawJson` | FString | 原始 JSON 字符串 |

**FMAAnnotationInput 结构体:**

| 字段 | 类型 | 说明 |
|------|------|------|
| `Id` | FString | 用户输入的 ID |
| `Type` | FString | 用户输入的类型 |
| `Shape` | EMAShapeType | 形状类型枚举 |
| `Properties` | TMap<FString, FString> | 额外属性 |

**EMAShapeType 枚举:**

| 值 | 说明 |
|----|------|
| `Point` | 点类型 (单选默认) |
| `Polygon` | 多边形类型 (多选建筑群) |
| `LineString` | 线串类型 (多选道路片段) |
| `Rectangle` | 矩形类型 (预留) |

### 16.3 MASceneGraphManager API

```cpp
// 解析用户输入
bool ParseLabelInput(const FString& Input, FString& OutId, FString& OutType, FString& OutError);

// 添加场景节点 (单选)
bool AddSceneNode(const FString& Input, const FVector& Location, AActor* Actor, FString& OutMessage);

// 生成标签 (Type-N 格式)
FString GenerateLabel(const FString& Type);

// 检查 ID 是否存在
bool IsIdExists(const FString& Id);

// 获取所有节点 (用于可视化)
TArray<FMASceneGraphNode> GetAllNodes() const;

// 通过 GUID 查询所属节点
TArray<FMASceneGraphNode> FindNodesByGuid(const FString& Guid) const;

// 加载/保存场景图
void LoadSceneGraph();
void SaveSceneGraph();
```

### 16.4 几何计算工具 (MAGeometryUtils)

```cpp
// 计算 2D 凸包 (Graham Scan 算法)
static TArray<FVector> ComputeConvexHull2D(const TArray<FVector>& Points);

// 收集 Actor 边界框角点
static TArray<FVector> CollectBoundingBoxCorners(const TArray<AActor*>& Actors);

// 最近邻排序 (用于 LineString)
static TArray<FVector> SortByNearestNeighbor(const TArray<FVector>& Points);

// 计算多边形几何中心
static FVector CalculatePolygonCentroid(const TArray<FVector>& Vertices);

// 计算线串几何中心
static FVector CalculateLineStringCentroid(const TArray<FVector>& Points);

// 2D 叉积
static float CrossProduct2D(const FVector& O, const FVector& A, const FVector& B);
```

### 16.5 JSON 文件格式

场景图数据保存在 `datasets/scene_graph_cyberworld.json`:

```json
{
  "nodes": [
    {
      "id": "building_01",
      "guid": "A1B2C3D4-E5F6-7890-ABCD-EF1234567890",
      "properties": {
        "type": "building",
        "label": "Building-1"
      },
      "shape": {
        "type": "point",
        "center": [100, 200, 0]
      }
    },
    {
      "id": "area_01",
      "count": 3,
      "Guid": ["GUID1", "GUID2", "GUID3"],
      "properties": {
        "type": "building",
        "label": "Building-2"
      },
      "shape": {
        "type": "polygon",
        "vertices": [[x1,y1,z1], [x2,y2,z2], [x3,y3,z3], [x4,y4,z4]]
      }
    },
    {
      "id": "road_01",
      "count": 5,
      "Guid": ["GUID1", "GUID2", "GUID3", "GUID4", "GUID5"],
      "properties": {
        "type": "road",
        "label": "Road-1"
      },
      "shape": {
        "type": "linestring",
        "points": [[x1,y1,z1], [x2,y2,z2], [x3,y3,z3], [x4,y4,z4], [x5,y5,z5]]
      }
    }
  ]
}
```

### 16.6 场景标签可视化

进入 Modify 模式时，系统自动在已标注 Actor 位置显示绿色文本标签：

```cpp
// MAHUD::DrawSceneLabels()
for (const FMASceneGraphNode& Node : CachedSceneNodes)
{
    FString DisplayText = FString::Printf(TEXT("GUID: %s\nLabel: %s"), 
        Node.Guid.IsEmpty() ? TEXT("N/A") : *Node.Guid,
        *Node.Label);
    
    FVector DisplayPos = Node.Center + FVector(0, 0, 100);  // Z 偏移 100
    DrawDebugString(GetWorld(), DisplayPos, DisplayText, nullptr, FColor::Green, 0.f, true);
}
```

**可视化生命周期:**
- `ShowModifyWidget()` → `StartSceneLabelVisualization()` → 加载 JSON 并开始显示
- `HideModifyWidget()` → `StopSceneLabelVisualization()` → 停止显示并清空缓存
- `DrawHUD()` 每帧调用 `DrawSceneLabels()` 刷新显示

### 16.7 GUID 反向查询

当用户点击 Actor 时，系统通过 GUID 查询该 Actor 所属的 Polygon/LineString 节点：

```cpp
// MAModifyWidget::UpdateJsonPreview()
FString ActorGuid = Actor->GetActorGuid().ToString();
TArray<FMASceneGraphNode> MatchingNodes = SceneGraphManager->FindNodesByGuid(ActorGuid);

if (MatchingNodes.Num() > 0)
{
    // 显示所属节点的完整 JSON
    JsonPreviewText->SetText(FText::FromString(MatchingNodes[0].RawJson));
}
else
{
    // 显示默认的单 Actor 预览
    ShowDefaultPreview(Actor);
}
```

### 16.8 错误处理

| 错误类型 | 处理方式 |
|---------|---------|
| 输入格式无效 | 显示错误消息，保留输入文本 |
| ID 已存在 | 显示警告消息，不写入 JSON |
| JSON 文件不存在 | 自动创建空结构 |
| JSON 解析失败 | 记录警告日志，跳过可视化 |
| Actor 为 nullptr | 生成空 GUID，记录警告 |


## 17. Edit Mode 系统 (编辑模式)

### 17.1 概述

Edit Mode 是一个用于模拟任务执行过程中"新情况"（动态变化）的交互模式。与 Modify Mode（持久化修改源场景图文件）不同，Edit Mode 的所有操作仅针对临时场景图文件进行，不影响源文件。该功能的核心目标是支持任务规划算法的验证：当场景中发生新情况时，系统需要能够触发重规划。

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Edit Mode 系统架构                                    │
│                                                                             │
│  ┌─────────────────┐                                                        │
│  │ MAPlayerController│  ◄── M 键切换模式                                     │
│  │ (Input 层)        │                                                      │
│  │ - CurrentMouseMode│  ← EMAMouseMode::Edit                                │
│  │ - EditModeManager │  ← 委托给 Manager 处理                                │
│  └────────┬─────────┘                                                       │
│           │                                                                 │
│           │ 模式切换事件                                                     │
│           ▼                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MAEditModeManager (UWorldSubsystem)               │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐      │   │
│  │  │ TempSceneGraph  │  │ POI Management  │  │ Selection Mgmt  │      │   │
│  │  │ 临时场景图管理  │  │ POI 创建/销毁   │  │ 选择集合管理    │      │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘      │   │
│  │                                                                      │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐      │   │
│  │  │ Node Operations │  │ Goal/Zone Mgmt  │  │ Backend Comm    │      │   │
│  │  │ 增删改节点      │  │ Goal/Zone Actor │  │ 后端通信接口    │      │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘      │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                   │                                         │
│                                   │ 场景变化通知                             │
│                                   ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MACommSubsystem                                   │   │
│  │  - SendSceneGraphChangeMessage()                                     │   │
│  │  - add_node / delete_node / edit_node / add_goal / add_zone         │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                   │                                         │
│                                   │ HTTP POST                               │
│                                   ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    Backend Planner                                   │   │
│  │                    (触发重规划)                                       │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 17.2 UI 架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Edit Mode UI 架构                                     │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MAHUD                                             │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐      │   │
│  │  │ Mode Indicator  │  │ MASceneListWidget│  │ MAEditWidget    │      │   │
│  │  │ 蓝色 "Edit (M)" │  │ (左侧面板)      │  │ (右侧面板)      │      │   │
│  │  │ 右上角          │  │ Goal/Zone 列表  │  │ JSON 编辑       │      │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘      │   │
│  │                                                                      │   │
│  │  ┌─────────────────┐                                                │   │
│  │  │ Coord Display   │                                                │   │
│  │  │ 绿色坐标显示    │                                                │   │
│  │  │ 屏幕下方        │                                                │   │
│  │  └─────────────────┘                                                │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 17.2.1 MAEditWidget 功能

MAEditWidget 是 Edit Mode 的主要编辑面板，位于屏幕右侧，提供以下功能：

| 功能区域 | 组件 | 说明 |
|---------|------|------|
| **Actor 操作区** | JSON 编辑框 | 编辑选中 Actor 对应节点的 JSON |
| | 确认按钮 | 提交 JSON 修改 |
| | 删除按钮 | 删除选中节点 |
| | 设为 Goal 按钮 | 将普通节点标记为 Goal |
| | 取消 Goal 按钮 | 取消节点的 Goal 标记 |
| **POI 操作区** | 描述输入框 | 输入 Goal/Zone 的描述信息 |
| | 创建 Goal 按钮 | 在 POI 位置创建 Goal 节点 |
| | 创建区域按钮 | 使用多个 POI 创建 Zone 节点 |
| | 删除 POI 按钮 | 删除选中的 POI |
| | 预设 Actor 下拉框 | 选择预设 Actor 类型 (预留) |
| | 添加按钮 | 在 POI 位置添加预设 Actor (预留) |

**选择状态与 UI 显示:**

| 选择状态 | Actor 操作区 | POI 操作区 | 提示文本 |
|---------|-------------|-----------|---------|
| 无选择 | 隐藏 | 隐藏 | "选中 Actor 或 POI 进行操作" |
| 选中 Actor | 显示 | 隐藏 | "编辑 JSON 后点击确认保存" |
| 选中 1 个 POI | 隐藏 | 显示 | "可创建 Goal 或添加预设 Actor" |
| 选中 2 个 POI | 隐藏 | 显示 | "选中 3 个以上 POI 可创建区域" |
| 选中 3+ 个 POI | 隐藏 | 显示 | "已选中 N 个 POI，可创建区域" |

### 17.3 MAEditModeManager API

```cpp
// 临时场景图管理
bool CreateTempSceneGraph();           // 从源文件复制创建临时文件
void DeleteTempSceneGraph();           // 删除临时文件
FString GetTempSceneGraphPath() const; // 获取临时文件路径
bool IsEditModeAvailable() const;      // 检查 Edit Mode 是否可用

// POI 管理
AMAPointOfInterest* CreatePOI(const FVector& WorldLocation);
void DestroyPOI(AMAPointOfInterest* POI);
void DestroyAllPOIs();
TArray<AMAPointOfInterest*> GetAllPOIs() const;

// 选择管理
void SelectActor(AActor* Actor);       // 单选 Actor
void SelectPOI(AMAPointOfInterest* POI); // 多选 POI
void DeselectObject(UObject* Object);
void ClearSelection();
AActor* GetSelectedActor() const;
TArray<AMAPointOfInterest*> GetSelectedPOIs() const;

// Node 操作
bool AddNode(const FString& NodeJson, FString& OutError);
bool DeleteNode(const FString& NodeId, FString& OutError);
bool EditNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError);
bool CreateGoal(const FVector& Location, const FString& Description, FString& OutError);
bool CreateZone(const TArray<FVector>& Vertices, const FString& Description, FString& OutError);

// Goal/Zone Actor 管理
AMAZoneActor* CreateZoneActor(const FString& NodeId, const TArray<FVector>& Vertices);
AMAGoalActor* CreateGoalActor(const FString& NodeId, const FVector& Location, const FString& Description);
void DestroyZoneActor(const FString& NodeId);
void DestroyGoalActor(const FString& NodeId);

// 设为 Goal 功能
bool SetNodeAsGoal(const FString& NodeId, FString& OutError);
bool UnsetNodeAsGoal(const FString& NodeId, FString& OutError);
bool IsNodeGoal(const FString& NodeId) const;

// 列表查询
TArray<FString> GetAllGoalNodeIds() const;
TArray<FString> GetAllZoneNodeIds() const;
FString GetNodeLabel(const FString& NodeId) const;
```

### 17.4 可视化 Actor

| Actor 类型 | 组件 | 外观 | 用途 |
|-----------|------|------|------|
| **AMAPointOfInterest** | UNiagaraComponent | 蓝色粒子效果 | 临时标记点 |
| **AMAGoalActor** | UStaticMeshComponent + UTextRenderComponent | 红色锥体 + 文本标签 | Goal 节点可视化 |
| **AMAZoneActor** | USplineComponent + USplineMeshComponent | 蓝色样条曲线边界 | Zone 节点可视化 |

### 17.5 数据流程

```
用户点击场景 → MAPlayerController::HandleEditModeClick()
                    │
                    ├── 左键点击空白区域 → EditModeManager->CreatePOI(Location)
                    │                           │
                    │                           └── 生成 AMAPointOfInterest Actor
                    │
                    └── Shift+左键点击 Actor → EditModeManager->SelectActor(Actor)
                                                    │
                                                    ├── 更新高亮状态
                                                    ├── 触发 OnSelectionChanged 委托
                                                    └── MAHUD 更新 MAEditWidget

用户点击 [创建 Goal] → MAEditWidget::OnCreateGoalButtonClicked()
                            │
                            ▼
                    EditModeManager->CreateGoal(Location, Description)
                            │
                            ├── 添加 Goal Node 到临时场景图
                            ├── 创建 AMAGoalActor 可视化
                            ├── 销毁对应 POI
                            ├── 发送 add_goal 消息到后端
                            └── 触发 OnTempSceneGraphChanged 委托

用户点击 [创建区域] → MAEditWidget::OnCreateZoneButtonClicked()
                            │
                            ▼
                    EditModeManager->CreateZone(Vertices, Description)
                            │
                            ├── 计算凸包
                            ├── 添加 Zone Node 到临时场景图
                            ├── 创建 AMAZoneActor 可视化
                            ├── 销毁所有参与的 POI
                            ├── 发送 add_zone 消息到后端
                            └── 触发 OnTempSceneGraphChanged 委托
```

### 17.6 场景变化消息类型

| 消息类型 | 枚举值 | Payload |
|---------|--------|---------|
| AddNode | add_node | 完整 Node JSON |
| DeleteNode | delete_node | Node ID |
| EditNode | edit_node | 修改后的 Node JSON |
| AddGoal | add_goal | Goal Node JSON |
| DeleteGoal | delete_goal | Goal ID |
| AddZone | add_zone | Zone Node JSON |
| DeleteZone | delete_zone | Zone ID |
| AddEdge | add_edge | Edge JSON |
| EditEdge | edit_edge | 修改后的 Edge JSON |

#### 17.6.1 消息发送规则

不同类型节点的操作会发送不同的消息组合：

| 操作 | 普通节点 | Goal 节点 (type=goal) | Zone 节点 (type=zone) | 普通节点 (is_goal=true) |
|------|---------|----------------------|----------------------|------------------------|
| 添加 | `add_node` | `add_node` + `add_goal` | `add_node` + `add_zone` | - |
| 删除 | `delete_node` | `delete_node` + `delete_goal` | `delete_node` + `delete_zone` | `delete_node` + `delete_goal` |
| 修改 | `edit_node` | `edit_node` | `edit_node` | `edit_node` |
| 设为 Goal | - | - | - | `edit_node` + `add_goal` |
| 取消 Goal | - | - | - | `edit_node` + `delete_goal` |

**说明:**
- `CreateGoal()` 内部调用 `AddNode()` 发送 `add_node`，然后额外发送 `add_goal`
- `CreateZone()` 内部调用 `AddNode()` 发送 `add_node`，然后额外发送 `add_zone`
- `DeleteNode()` 检测节点类型：
  - 如果是 Goal/Zone 类型 (properties.type)，在发送 `delete_node` 后额外发送 `delete_goal`/`delete_zone`
  - 如果是普通节点但有 `is_goal: true`，在发送 `delete_node` 后额外发送 `delete_goal`
- `SetNodeAsGoal()` 发送 `edit_node` + `add_goal`
- `UnsetNodeAsGoal()` 发送 `edit_node` + `delete_goal`

### 17.7 临时场景图文件

临时场景图文件存储在 `Saved/Temp/` 目录下，文件名包含时间戳：

```
Saved/Temp/temp_scene_graph_20251230_150939.json
```

**生命周期:**
- 游戏启动时从源文件 `datasets/scene_graph_cyberworld.json` 复制创建
- Edit Mode 的所有修改仅影响临时文件
- 游戏结束时自动删除

### 17.8 选择机制约束

| 约束 | 说明 |
|------|------|
| POI 多选 | 可同时选中多个 POI |
| Actor 单选 | 同一时间只能选中一个 Actor |
| 互斥选择 | Actor 和 POI 不能同时选中 |

### 17.9 Shape 类型编辑约束

| Shape 类型 | 可编辑字段 | 可删除 |
|------------|-----------|--------|
| point | properties, shape.center | ✓ |
| polygon | properties | ✗ |
| linestring | properties | ✗ |

### 17.10 错误处理

| 错误类型 | 处理策略 |
|---------|---------|
| 源文件不存在 | 禁用 Edit Mode，记录错误日志 |
| 临时文件创建失败 | 禁用 Edit Mode，显示通知 |
| JSON 格式不合法 | 显示错误提示，阻止提交 |
| Node 操作失败 | 记录日志，显示通知，不修改文件 |
| 后端通信失败 | 本地操作生效，显示警告 |
| 非法删除操作 | 显示错误提示，阻止删除 |

### 17.11 Edit Mode 操作说明

| 操作 | 功能 |
|------|------|
| **M 键** | 循环切换模式 (Select → Deployment → Modify → Edit → Select) |
| **左键点击空白区域** | 创建 POI (蓝色粒子标记点) |
| **左键点击 POI** | 选中/取消选中 POI (支持多选) |
| **Shift+左键点击 Actor** | 选中 Actor 进行编辑 |
| **左键点击 Goal/Zone Actor** | 选中对应节点进行编辑 |

### 17.12 Goal/Zone 创建流程

**创建 Goal:**
1. 在场景中点击创建一个 POI
2. 选中该 POI
3. 在描述输入框中输入描述信息
4. 点击"创建 Goal"按钮
5. 系统自动：创建 Goal 节点 → 生成 GoalActor 可视化 → 销毁 POI → 发送消息到后端

**创建 Zone:**
1. 在场景中点击创建 3 个或更多 POI
2. 依次选中所有 POI (支持多选)
3. 在描述输入框中输入描述信息
4. 点击"创建区域"按钮
5. 系统自动：计算凸包 → 创建 Zone 节点 → 生成 ZoneActor 可视化 → 销毁所有 POI → 发送消息到后端

### 17.13 设为 Goal 功能

对于已存在的普通节点 (非 Goal/Zone 类型)，可以将其标记为 Goal：

1. 选中一个普通 Actor (对应 point 类型节点)
2. 点击"设为 Goal"按钮
3. 系统自动：在节点 JSON 中添加 `is_goal: true` → 创建 GoalActor 可视化 → 发送 `edit_node` + `add_goal` 消息

取消 Goal 标记：
1. 选中一个已标记为 Goal 的节点
2. 点击"取消 Goal"按钮
3. 系统自动：移除 `is_goal` 字段 → 销毁 GoalActor → 发送 `edit_node` + `delete_goal` 消息



# 系统架构

## 概述

MultiAgent 是一个基于 UE5 的多机器人仿真系统，支持多种类型机器人的协同任务执行。系统采用三层架构设计，通过通信接口与外部规划器（Python 端）交互，实现自然语言指令到机器人动作的转换。

## 系统架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           外部系统 (Python 端)                               │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                      │
│  │ 自然语言处理 │ -> │  任务规划器  │ -> │  技能列表   │                      │
│  └─────────────┘    └─────────────┘    └─────────────┘                      │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │ HTTP/JSON
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           UE5 仿真端                                         │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                    通信接口 (MACommSubsystem)                          │ │
│  │         HTTP 轮询 │ 消息封装 │ 世界状态响应 │ 反馈发送                 │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                    │                                        │
│  ══════════════════════════════════╪════════════════════════════════════   │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                 第一层: 指令调度层 (MACommandManager)                │   │
│  │      技能列表解析 │ 时间步调度 │ 参数预处理 │ 反馈收集               │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                 第二层: 技能管理层 (MASkillComponent)                │   │
│  │      技能参数管理 │ StateTree Tag 触发 │ 反馈上下文 │ 能量系统       │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                 第三层: 技能内核层 (GameplayAbility)                 │   │
│  │   SK_Navigate │ SK_Search │ SK_Follow │ SK_Place │ SK_Charge │ ...  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 三层架构详解

### 第一层: 指令调度层 (MACommandManager)

**职责**：接收外部指令，进行参数预处理，调度技能执行，收集反馈。

```cpp
// 核心接口
void ExecuteSkillList(const FMASkillListMessage& SkillList);  // 执行技能列表
void SendCommand(AMACharacter* Agent, EMACommand Command);     // 发送单个指令
```

**时间步执行流程**：
1. 接收 Python 端发送的技能列表（按时间步组织）
2. 顺序执行每个时间步的所有指令
3. 等待当前时间步所有技能完成
4. 收集反馈并发送给 Python 端
5. 执行下一个时间步

**支持的指令类型**：
| 指令 | 枚举值 | 说明 |
|------|--------|------|
| Navigate | EMACommand::Navigate | 导航到目标位置 |
| Search | EMACommand::Search | 区域搜索 |
| Follow | EMACommand::Follow | 跟随目标 |
| Place | EMACommand::Place | 放置物体 |
| Charge | EMACommand::Charge | 充电 |
| TakeOff | EMACommand::TakeOff | 起飞 |
| Land | EMACommand::Land | 降落 |
| ReturnHome | EMACommand::ReturnHome | 返航 |

### 第二层: 技能管理层 (MASkillComponent)

**职责**：管理单个机器人的技能参数、激活/取消技能、收集反馈上下文。

```cpp
// 技能激活接口
bool TryActivateNavigate(FVector TargetLocation);
bool TryActivateSearch();
bool TryActivateFollow(AMACharacter* Target, float Distance);
bool TryActivatePlace();
bool TryActivateCharge();
bool TryActivateTakeOff();
bool TryActivateLand();
bool TryActivateReturnHome();

// 技能取消
void CancelAllSkills();
```

**技能参数结构 (FMASkillParams)**：
```cpp
struct FMASkillParams
{
    // Navigate
    FVector TargetLocation;
    float AcceptanceRadius;
    
    // Follow
    TWeakObjectPtr<AMACharacter> FollowTarget;
    float FollowDistance;
    
    // Search
    TArray<FVector> SearchBoundary;      // 搜索区域边界
    FMASemanticTarget SearchTarget;       // 搜索目标语义
    float SearchScanWidth;
    
    // Place
    FMASemanticTarget PlaceObject1;       // 被放置物体
    FMASemanticTarget PlaceObject2;       // 放置位置
    
    // TakeOff / Land / ReturnHome
    float TakeOffHeight;
    float LandHeight;
    FVector HomeLocation;
};
```

**能量系统**：
- `Energy` / `MaxEnergy` - 当前/最大能量
- `EnergyDrainRate` - 能量消耗速率
- `LowEnergyThreshold` - 低电量阈值
- `OnEnergyDepleted` - 能量耗尽委托

### 第三层: 技能内核层 (GameplayAbility)

**职责**：实现具体的技能逻辑，基于 UE5 GameplayAbility 系统。

| 技能类 | 功能 | 导航方式 |
|--------|------|----------|
| SK_Navigate | 点对点导航 | 地面: AIController + NavMesh<br>飞行: 直线飞行 |
| SK_Search | 区域搜索 | 割草机航线 (Lawnmower Pattern) |
| SK_Follow | 跟随目标 | 实时追踪 + 距离保持 |
| SK_Place | 放置物体 | 语义查询 + 导航 + 放置动作 |
| SK_Charge | 充电 | 导航到充电站 + 充电等待 |
| SK_TakeOff | 起飞 | 垂直上升到指定高度 |
| SK_Land | 降落 | 垂直下降到地面 |
| SK_ReturnHome | 返航 | 导航到出生点 |

**技能生命周期**：
```
ActivateAbility() -> 执行逻辑 -> EndAbility() -> NotifySkillCompleted()
```

## 机器人类型与技能

| 类型 | 类名 | 可用技能 | 导航方式 |
|------|------|----------|----------|
| UAV (多旋翼无人机) | MAUAVCharacter | Navigate, Search, Follow, TakeOff, Land, Charge | 3D 飞行 |
| FixedWingUAV (固定翼无人机) | MAFixedWingUAVCharacter | Navigate, Search, TakeOff, Land, Charge | 3D 飞行 |
| UGV (无人车) | MAUGVCharacter | Navigate, Carry, Charge | NavMesh 地面导航 |
| Quadruped (四足机器人) | MAQuadrupedCharacter | Navigate, Search, Follow, Charge | NavMesh 地面导航 |
| Humanoid (人形机器人) | MAHumanoidCharacter | Navigate, Place, Charge | NavMesh 地面导航 |

## 通信接口 (MACommSubsystem)

通信子系统作为 UE5 与外部系统的接口，负责消息的收发和序列化。

### 消息信封格式

```json
{
    "message_type": "skill_list",
    "timestamp": 1703836800000,
    "message_id": "uuid-string",
    "payload": { ... }
}
```

### 消息类型

| 方向 | 类型 | 说明 |
|------|------|------|
| 出站 | UIInput | UI 输入消息 |
| 出站 | ButtonEvent | 按钮事件 |
| 出站 | TaskFeedback | 任务反馈 |
| 出站 | WorldState | 世界状态响应 |
| 入站 | SkillList | 技能列表 |
| 入站 | TaskPlanDAG | 任务规划 DAG |
| 入站 | WorldModelGraph | 世界模型图 |
| 入站 | QueryRequest | 查询请求 |

### 技能列表格式

```json
{
    "0": {
        "UAV_0": { "skill": "navigate", "params": { "dest_position": [1000, 2000, 500] } },
        "UGV_0": { "skill": "navigate", "params": { "dest_position": [500, 500, 0] } }
    },
    "1": {
        "UAV_0": { "skill": "search", "params": { "search_area": [[0,0], [1000,0], [1000,1000], [0,1000]] } }
    }
}
```

### 反馈格式

```json
{
    "time_step": 0,
    "feedbacks": [
        {
            "agent_id": "UAV_0",
            "skill_name": "navigate",
            "success": true,
            "message": "Navigate succeeded: Reached destination (1000, 2000, 500)",
            "data": { "final_position": "(1000, 2000, 500)" }
        }
    ]
}
```

## 核心子系统

### 游戏流程
| 类 | 职责 |
|----|------|
| MAGameInstance | 全局配置加载、Setup UI 状态、跨关卡数据 |
| MAGameMode | 仿真地图初始化、Agent 生成调度、观察者设置 |
| MAConfigManager | 统一配置加载 (simulation/agents/environment.json) |

### 管理器
| 类 | 职责 |
|----|------|
| MAAgentManager | Agent 生命周期、按类型/ID 查询、批量操作 |
| MACommandManager | 指令调度、时间步执行、反馈收集 |
| MASelectionManager | 选择状态管理 |
| MASquadManager | 编队管理 |
| MAViewportManager | 视口管理 |

### 工具类
| 类 | 职责 |
|----|------|
| MAWorldQuery | 世界状态查询、实体/边界特征提取 |
| MAFeedbackGenerator | 统一反馈生成 |
| MAGeometryUtils | 几何计算 (割草机航线等) |
| MASceneQuery | 场景语义查询 |

## 配置文件

配置文件位于 `config/` 目录：

| 文件 | 内容 |
|------|------|
| simulation.json | 服务器地址、轮询设置、观察者位置 |
| agents.json | 机器人类型定义、实例列表 |
| environment.json | 充电站、道具配置 |

## 文件结构

```
unreal_project/Source/MultiAgent/
├── Core/                           # 核心子系统
│   ├── Config/
│   │   └── MAConfigManager         # 配置管理器
│   ├── GameFlow/
│   │   ├── MAGameInstance          # 游戏实例
│   │   ├── MAGameMode              # 仿真游戏模式
│   │   └── MASetupGameMode         # 设置游戏模式
│   ├── Manager/
│   │   ├── MAAgentManager          # Agent 管理
│   │   ├── MACommandManager        # 指令调度
│   │   ├── MASelectionManager      # 选择管理
│   │   ├── MASquadManager          # 编队管理
│   │   └── MAViewportManager       # 视口管理
│   ├── Comm/
│   │   ├── MACommSubsystem         # 通信子系统
│   │   └── MACommTypes             # 通信类型定义
│   └── Types/
│       ├── MATypes                 # 基础类型
│       ├── MASimTypes              # 仿真类型
│       └── MATaskGraphTypes        # 任务图类型
│
├── Agent/
│   ├── Character/                  # 机器人角色
│   │   ├── MACharacter             # 基类
│   │   ├── MAUAVCharacter          # 多旋翼无人机
│   │   ├── MAFixedWingUAVCharacter # 固定翼无人机
│   │   ├── MAUGVCharacter          # 无人车
│   │   ├── MAQuadrupedCharacter    # 四足机器人
│   │   └── MAHumanoidCharacter     # 人形机器人
│   ├── Skill/
│   │   ├── MASkillComponent        # 技能管理组件
│   │   ├── MASkillTags             # GameplayTag 定义
│   │   ├── MAFeedbackSystem        # 反馈模板系统
│   │   ├── Impl/                   # 技能实现
│   │   │   ├── SK_Navigate
│   │   │   ├── SK_Search
│   │   │   ├── SK_Follow
│   │   │   ├── SK_Place
│   │   │   ├── SK_Charge
│   │   │   ├── SK_TakeOff
│   │   │   ├── SK_Land
│   │   │   └── SK_ReturnHome
│   │   └── Utils/
│   │       ├── MAFeedbackGenerator
│   │       ├── MAGeometryUtils
│   │       ├── MASceneQuery
│   │       └── MASkillParamsProcessor
│   ├── StateTree/
│   │   ├── MAStateTreeComponent
│   │   ├── Task/
│   │   └── Condition/
│   └── Component/
│       └── Sensor/
│
├── Environment/
│   └── MAChargingStation
│
├── Input/
│   └── MAPlayerController
│
├── UI/
│   ├── MAHUD
│   └── MAMiniMapManager
│
└── Utils/
    └── MAWorldQuery
```

## 数据流

### 指令执行流程

```
Python 端发送 SkillList
        │
        ▼
MACommSubsystem.HandlePollResponse()  ← 通信接口
        │
        ▼
MACommandManager.ExecuteSkillList()   ← 第一层: 指令调度
        │
        ▼
MASkillComponent.TryActivateXxx()     ← 第二层: 技能管理
        │
        ▼
SK_Xxx.ActivateAbility()              ← 第三层: 技能内核
        │
        ▼
SK_Xxx.EndAbility() + NotifySkillCompleted()
        │
        ▼
MACommandManager.OnSkillCompleted()   ← 反馈收集
        │
        ▼
MACommSubsystem.SendTimeStepFeedback() ← 通信接口
        │
        ▼
Python 端接收反馈
```

## 扩展指南

### 添加新机器人类型

1. 创建新的 Character 类继承 `AMACharacter`
2. 在 `MAAgentManager::GetClassPathForType()` 添加类型映射
3. 在 `agents.json` 的 `agent_types` 中配置蓝图路径
4. 根据需要重写导航方法 (`TryNavigateTo`, `StopNavigation`)

### 添加新技能

1. 创建新的 GameplayAbility 类继承 `UGameplayAbility`
2. 在 `MASkillTags` 中添加对应的 GameplayTag
3. 在 `MASkillComponent` 中添加激活/取消接口
4. 在 `MACommandManager` 中添加指令类型和 Tag 映射
5. 在 `MAFeedbackGenerator` 中添加反馈生成逻辑

### 添加新的通信消息类型

1. 在 `MACommTypes.h` 中定义新的消息结构
2. 在 `EMACommMessageType` 枚举中添加类型
3. 在 `MACommSubsystem::HandlePollResponse()` 中添加处理逻辑



