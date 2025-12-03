# MultiAgent-Unreal 架构设计

## 1. 系统概述

MultiAgent-Unreal 是一个基于 Unreal Engine 5 的多智能体仿真框架，用于机器人、无人机、角色等异构智能体的协同仿真。

## 2. Manager 子系统设计

| Manager | 中文名 | 功能介绍 | UE/C++ 实现方案 | 优先级 |
|---------|-------|---------|----------------|-------|
| **AgentManager** | 司命 | 智能体生命周期管理：spawn、destroy、查询、按类型分组 | 扩展 `AMAGameMode`，已有基础实现 | ⭐⭐⭐ P0 |
| **RelationManager** | 司缘 | 智能体关系管理：追踪、跟随、视野、碰撞等关系图 | 自定义 `UMARelationSubsystem` + 图数据结构 | ⭐⭐⭐ P0 |
| **MapManager** | 司图 | 地图感知：区域划分、兴趣点、导航网格查询 | `UNavigationSystemV1` + 自定义感知层 | ⭐⭐ P1 |
| **ConfigManager** | 司书 | 配置管理：Agent 配置、场景配置、运行时参数 | `UDataTable` + `UDataAsset` + `UDeveloperSettings` | ⭐⭐ P1 |
| **CommunityManager** | 司通 | 智能体通信：消息传递、广播、组播 | `UGameplayMessageSubsystem` + Delegates | ⭐ P2 |
| **ResourceManager** | 司廪 | 资源管理：能量、弹药、负载等属性 | `UGameplayAbilitySystem (GAS)` AttributeSet | ⭐ P2 |
| **DisplayManager** | 司显 | 状态显示：HUD、调试可视化、状态面板 | `UMG` + `AHUD` + Debug Draw | ⭐ P2 |
| **AssetManager** | 司庾 | 资产加载：模型、动画、材质的异步加载 | 直接使用 `UAssetManager`，**无需开发** | ❌ 不需要 |

## 3. 优先级说明

- **P0 (必须)**: 核心功能，系统运行的基础
- **P1 (重要)**: 提升系统能力，建议尽早开发
- **P2 (可选)**: 增强功能，按需开发
- **不需要**: UE 自带功能完全满足需求

## 4. 当前已实现的类

| 类名 | 文件名 | 简介 |
|-----|-------|-----|
| AMAAgent | MAAgent.h/cpp | Agent 基类，提供移动、ID、类型等基础功能 |
| AMAHumanAgent | MAHumanAgent.h/cpp | 人类 Agent，使用 Manny 模型和动画蓝图 |
| AMARobotDogAgent | MARobotDogAgent.h/cpp | 机器狗 Agent，带行走/待机动画切换 |
| AMACharacter | MACharacter.h/cpp | 带俯视摄像机的角色，用于玩家控制 |
| AMAGameMode | MAGameMode.h/cpp | 游戏模式，负责 spawn 和管理所有 Agent |
| AMAPlayerController | MAPlayerController.h/cpp | 玩家控制器，处理鼠标点击移动逻辑 |
| EAgentType | MAAgent.h | Agent 类型枚举 (Human, RobotDog, Drone) |

## 5. 类继承关系

```
ACharacter (UE)
    └── AMAAgent (Agent 基类)
            ├── AMAHumanAgent (人类)
            └── AMARobotDogAgent (机器狗)
    └── AMACharacter (带摄像机的角色)

AGameModeBase (UE)
    └── AMAGameMode (AgentManager 的载体)

APlayerController (UE)
    └── AMAPlayerController
```

## 6. 建议的开发路线

### Phase 1: 核心框架 (P0)
1. 完善 `AMAGameMode` 作为 AgentManager
   - Agent 注册/注销
   - 按类型/ID 查询
   - 批量操作接口

2. 实现 `UMARelationSubsystem` 作为 RelationManager
   - 关系类型枚举 (Tracking, Following, Near, Visible)
   - 关系图数据结构
   - 关系查询 API

### Phase 2: 扩展功能 (P1)
3. MapManager - 地图感知层
4. ConfigManager - 运行时配置

### Phase 3: 增强功能 (P2)
5. CommunityManager - 智能体通信
6. ResourceManager - 资源属性
7. DisplayManager - 调试可视化

## 7. 文件结构规划

```
unreal_project/Source/MultiAgent/
├── AgentManager/                    # AgentManager 模块 (司命)
│   ├── MAAgentManager.h/cpp         # AgentManager 核心类
│   ├── MAAgent.h/cpp                # Agent 基类
│   ├── MAHumanAgent.h/cpp           # 人类 Agent
│   ├── MARobotDogAgent.h/cpp        # 机器狗 Agent
│   └── MACharacter.h/cpp            # 带摄像机的角色
├── Core/                            # 核心框架
│   ├── MAGameMode.h/cpp             # 游戏模式 (持有 AgentManager)
│   └── MAPlayerController.h/cpp     # 玩家控制器
├── RelationManager/                 # RelationManager 模块 (司缘) - 待开发
├── MapManager/                      # MapManager 模块 (司图) - 待开发
└── MultiAgent.h/cpp                 # 模块定义
```

## 8. AgentManager 详细设计

### 8.1 设计思路

AgentManager 负责所有智能体的生命周期管理。

**方案 A: 独立的 UMAAgentManager 类** (最终选定方案)
- 作为 `UWorldSubsystem` 或 `UGameInstanceSubsystem`
- GameMode 持有并初始化它
- 优点：职责单一，可复用
- 缺点：多一层间接调用



### 8.2 AgentManager 功能列表

| 功能 | 方法 | 说明 | 状态 |
|-----|------|-----|------|
| **生成 Agent** | `SpawnAgent()` | 根据类型和位置生成 Agent | ✅ 已实现 |
| **销毁 Agent** | `DestroyAgent()` | 销毁指定 Agent | ❌ 待实现 |
| **获取所有 Agent** | `GetAllAgents()` | 返回所有 Agent 列表 | ✅ 已实现 |
| **按类型查询** | `GetAgentsByType()` | 按 EAgentType 筛选 | ✅ 已实现 |
| **按 ID 查询** | `GetAgentByID()` | 根据 AgentID 查找 | ❌ 待实现 |
| **按名称查询** | `GetAgentByName()` | 根据 AgentName 查找 | ❌ 待实现 |
| **获取 Agent 数量** | `GetAgentCount()` | 返回当前 Agent 总数 | ❌ 待实现 |
| **批量移动** | `MoveAllAgentsTo()` | 所有 Agent 移动到目标点 | ✅ 已实现 (在 PlayerController) |
| **批量停止** | `StopAllAgents()` | 停止所有 Agent 移动 | ❌ 待实现 |
| **注册回调** | `OnAgentSpawned` | Agent 生成时的委托 | ❌ 待实现 |
| **注册回调** | `OnAgentDestroyed` | Agent 销毁时的委托 | ❌ 待实现 |

### 8.3 Agent 类层次

| 类 | 父类 | 用途 | 文件位置 |
|---|------|-----|---------|
| `AMAAgent` | ACharacter | Agent 基类，定义通用接口 | AgentManager/MAAgent.h |
| `AMAHumanAgent` | AMAAgent | 人形角色，使用骨骼动画 | AgentManager/MAHumanAgent.h |
| `AMARobotDogAgent` | AMAAgent | 四足机器人，自定义动画切换 | AgentManager/MARobotDogAgent.h |
| `AMADroneAgent` | AMAAgent | 无人机 (待实现) | AgentManager/MADroneAgent.h |
| `AMACharacter` | ACharacter | 玩家控制的角色，带摄像机 | AgentManager/MACharacter.h |

### 8.4 EAgentType 枚举

```cpp
UENUM(BlueprintType)
enum class EAgentType : uint8
{
    Human,      // 人类
    RobotDog,   // 机器狗
    Drone,      // 无人机
    Vehicle,    // 车辆 (待扩展)
    Animal      // 动物 (待扩展)
};
```
