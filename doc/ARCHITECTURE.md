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
