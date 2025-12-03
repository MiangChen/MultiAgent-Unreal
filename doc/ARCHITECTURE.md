# MultiAgent-Unreal 架构设计

## 1. 系统概述

MultiAgent-Unreal 是一个基于 Unreal Engine 5 的多智能体仿真框架，用于机器人、无人机、角色等异构智能体的协同仿真。

## 2. 当前文件结构

```
unreal_project/Source/MultiAgent/
├── AgentManager/                    # AgentManager 模块 (司命) ✅ 已实现
│   ├── MAAgentSubsystem.h/cpp       # AgentManager 核心类 (UWorldSubsystem)
│   ├── MAAgent.h/cpp                # Agent 基类
│   ├── MAHumanAgent.h/cpp           # 人类 Agent
│   ├── MARobotDogAgent.h/cpp        # 机器狗 Agent
│   └── MACharacter.h/cpp            # 带摄像机的角色
├── Core/                            # 核心框架
│   ├── MAGameMode.h/cpp             # 游戏模式 (协调各子系统)
│   └── MAPlayerController.h/cpp     # 玩家控制器
├── RelationManager/                 # RelationManager 模块 (司缘) - 待开发
├── MapManager/                      # MapManager 模块 (司图) - 待开发
└── MultiAgent.h/cpp                 # 模块定义
```

## 3. Manager 子系统设计

| Manager | 中文名 | 功能介绍 | UE/C++ 实现方案 | 优先级 | 状态 |
|---------|-------|---------|----------------|-------|------|
| **AgentManager** | 司命 | 智能体生命周期管理：spawn、destroy、查询、按类型分组 | `UMAAgentSubsystem` (UWorldSubsystem) | ⭐⭐⭐ P0 | ✅ 已实现 |
| **RelationManager** | 司缘 | 智能体关系管理：追踪、跟随、视野、碰撞等关系图 | `UMARelationSubsystem` + 图数据结构 | ⭐⭐⭐ P0 | ❌ 待开发 |
| **MapManager** | 司图 | 地图感知：区域划分、兴趣点、导航网格查询 | `UNavigationSystemV1` + 自定义感知层 | ⭐⭐ P1 | ❌ 待开发 |
| **ConfigManager** | 司书 | 配置管理：Agent 配置、场景配置、运行时参数 | `UDataTable` + `UDataAsset` | ⭐⭐ P1 | ❌ 待开发 |
| **CommunityManager** | 司通 | 智能体通信：消息传递、广播、组播 | `UGameplayMessageSubsystem` + Delegates | ⭐ P2 | ❌ 待开发 |
| **ResourceManager** | 司廪 | 资源管理：能量、弹药、负载等属性 | `GAS` AttributeSet | ⭐ P2 | ❌ 待开发 |
| **DisplayManager** | 司显 | 状态显示：HUD、调试可视化、状态面板 | `UMG` + `AHUD` | ⭐ P2 | ❌ 待开发 |
| **AssetManager** | 司庾 | 资产加载：模型、动画、材质的异步加载 | 直接使用 `UAssetManager` | ❌ 不需要 | - |

## 4. 类一览表

### 4.1 AgentManager 模块

| 类名 | 文件位置 | 简介 |
|-----|---------|-----|
| `UMAAgentSubsystem` | AgentManager/MAAgentSubsystem.h | Agent 管理子系统，负责生命周期管理 |
| `AMAAgent` | AgentManager/MAAgent.h | Agent 基类，提供移动、ID、类型等基础功能 |
| `AMAHumanAgent` | AgentManager/MAHumanAgent.h | 人类 Agent，使用 Manny 模型和动画蓝图 |
| `AMARobotDogAgent` | AgentManager/MARobotDogAgent.h | 机器狗 Agent，带行走/待机动画切换 |
| `AMACharacter` | AgentManager/MACharacter.h | 带俯视摄像机的角色，用于玩家控制 |
| `EAgentType` | AgentManager/MAAgent.h | Agent 类型枚举 (Human, RobotDog, Drone) |

### 4.2 Core 模块

| 类名 | 文件位置 | 简介 |
|-----|---------|-----|
| `AMAGameMode` | Core/MAGameMode.h | 游戏模式，协调各子系统，初始化场景 |
| `AMAPlayerController` | Core/MAPlayerController.h | 玩家控制器，处理鼠标点击移动逻辑 |

## 5. 类继承关系

```
UWorldSubsystem (UE)
    └── UMAAgentSubsystem (AgentManager 司命)

ACharacter (UE)
    └── AMAAgent (Agent 基类)
            ├── AMAHumanAgent (人类)
            └── AMARobotDogAgent (机器狗)
    └── AMACharacter (带摄像机的角色)

AGameModeBase (UE)
    └── AMAGameMode (游戏模式)

APlayerController (UE)
    └── AMAPlayerController
```

## 6. UMAAgentSubsystem 功能列表

| 功能 | 方法 | 说明 | 状态 |
|-----|------|-----|------|
| **生成 Agent** | `SpawnAgent()` | 根据类型和位置生成 Agent | ✅ |
| **销毁 Agent** | `DestroyAgent()` | 销毁指定 Agent | ✅ |
| **获取所有 Agent** | `GetAllAgents()` | 返回所有 Agent 列表 | ✅ |
| **按类型查询** | `GetAgentsByType()` | 按 EAgentType 筛选 | ✅ |
| **按 ID 查询** | `GetAgentByID()` | 根据 AgentID 查找 | ✅ |
| **按名称查询** | `GetAgentByName()` | 根据 AgentName 查找 | ✅ |
| **获取 Agent 数量** | `GetAgentCount()` | 返回当前 Agent 总数 | ✅ |
| **批量移动** | `MoveAllAgentsTo()` | 所有 Agent 移动到目标点 | ✅ |
| **批量停止** | `StopAllAgents()` | 停止所有 Agent 移动 | ✅ |
| **生成回调** | `OnAgentSpawned` | Agent 生成时的委托 | ✅ |
| **销毁回调** | `OnAgentDestroyed` | Agent 销毁时的委托 | ✅ |

## 7. 使用示例

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

## 8. 测试按键

运行游戏后可用以下按键测试：

| 按键 | 功能 |
|-----|------|
| **T** | 在玩家前方生成一个机器狗 |
| **Y** | 打印当前所有 Agent 信息（数量、类型、位置） |
| **U** | 销毁最后一个 Agent |
| 左键 | 移动玩家 |
| 右键 | 移动所有 Agent 到点击位置 |

## 9. 开发路线

### Phase 1: 核心框架 (P0) ✅ 已完成
- [x] UMAAgentSubsystem - Agent 生命周期管理
- [x] AMAAgent 及其子类
- [x] AMAGameMode 集成

### Phase 2: 关系管理 (P0)
- [ ] UMARelationSubsystem - 实体关系图
- [ ] 关系类型枚举 (Tracking, Following, Near, Visible)
- [ ] 关系查询 API

### Phase 3: 扩展功能 (P1)
- [ ] MapManager - 地图感知层
- [ ] ConfigManager - 运行时配置

### Phase 4: 增强功能 (P2)
- [ ] CommunityManager - 智能体通信
- [ ] ResourceManager - 资源属性
- [ ] DisplayManager - 调试可视化
