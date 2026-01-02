# MultiAgent-Unreal 架构设计

## 1. 系统概述

MultiAgent 是一个基于 UE5 的多机器人仿真系统，支持多种类型机器人的协同任务执行。系统采用三层架构设计，通过通信接口与外部规划器（Python 端）交互，实现自然语言指令到机器人动作的转换。

## 2. 系统架构图

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


## 3. 三层架构详解

### 3.1 第一层: 指令调度层 (MACommandManager)

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

### 3.2 第二层: 技能管理层 (MASkillComponent)

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


### 3.3 第三层: 技能内核层 (GameplayAbility)

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

## 4. 机器人类型与技能

| 类型 | 类名 | 可用技能 | 导航方式 |
|------|------|----------|----------|
| UAV (多旋翼无人机) | MAUAVCharacter | Navigate, Search, Follow, TakeOff, Land, Charge | 3D 飞行 |
| FixedWingUAV (固定翼无人机) | MAFixedWingUAVCharacter | Navigate, Search, TakeOff, Land, Charge | 3D 飞行 |
| UGV (无人车) | MAUGVCharacter | Navigate, Carry, Charge | NavMesh 地面导航 |
| Quadruped (四足机器人) | MAQuadrupedCharacter | Navigate, Search, Follow, Charge | NavMesh 地面导航 |
| Humanoid (人形机器人) | MAHumanoidCharacter | Navigate, Place, Charge | NavMesh 地面导航 |

## 5. 文件结构

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
│   │   ├── MAViewportManager       # 视口管理
│   │   ├── MASceneGraphManager     # 场景图管理
│   │   ├── MAEditModeManager       # Edit 模式管理
│   │   └── MAEmergencyManager      # 突发事件管理
│   ├── Comm/
│   │   ├── MACommSubsystem         # 通信子系统
│   │   └── MACommTypes             # 通信类型定义
│   └── Types/
│       ├── MATypes                 # 基础类型
│       ├── MASimTypes              # 仿真类型
│       ├── MASceneGraphTypes       # 场景图类型
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
│   │   ├── MASkillBase             # 技能基类
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
│   │       ├── MASkillGeometryUtils
│   │       ├── MASceneQuery
│   │       └── MASkillParamsProcessor
│   ├── StateTree/
│   │   ├── MAStateTreeComponent
│   │   ├── Task/
│   │   └── Condition/
│   └── Component/
│       └── Sensor/
│           ├── MASensorComponent
│           └── MACameraSensorComponent
│
├── Environment/
│   ├── MAChargingStation
│   ├── MAPatrolPath
│   ├── MACoverageArea
│   ├── MAPickupItem
│   ├── MAPointOfInterest
│   ├── MAGoalActor
│   └── MAZoneActor
│
├── Input/
│   ├── MAPlayerController
│   ├── MAInputActions
│   └── MAAgentInputComponent
│
├── UI/
│   ├── MAHUD
│   ├── MASimpleMainWidget
│   ├── MAEmergencyWidget
│   ├── MAModifyWidget
│   ├── MAEditWidget
│   ├── MASceneListWidget
│   ├── MATaskPlannerWidget
│   ├── MADAGCanvasWidget
│   ├── MATaskNodeWidget
│   ├── MANodePaletteWidget
│   ├── MAMiniMapWidget
│   └── MAMiniMapManager
│
└── Utils/
    ├── MAWorldQuery
    └── MAGeometryUtils
```


## 6. 核心子系统

### 6.1 游戏流程
| 类 | 职责 |
|----|------|
| MAGameInstance | 全局配置加载、Setup UI 状态、跨关卡数据 |
| MAGameMode | 仿真地图初始化、Agent 生成调度、观察者设置 |
| MAConfigManager | 统一配置加载 (simulation/agents/environment.json) |

### 6.2 管理器
| 类 | 职责 |
|----|------|
| MAAgentManager | Agent 生命周期、按类型/ID 查询、批量操作 |
| MACommandManager | 指令调度、时间步执行、反馈收集 |
| MASelectionManager | 选择状态管理、框选、编组 (Ctrl+1~9) |
| MASquadManager | 编队管理、Squad 创建/解散 |
| MAViewportManager | 视口管理、Agent View Mode (Direct Control) |
| MASceneGraphManager | 场景图管理、语义标注、JSON 持久化 |
| MAEditModeManager | Edit 模式管理、临时场景图、POI/Goal/Zone 管理 |
| MAEmergencyManager | 突发事件触发/结束、状态管理 |

### 6.3 工具类
| 类 | 职责 |
|----|------|
| MAWorldQuery | 世界状态查询、实体/边界特征提取 |
| MAFeedbackGenerator | 统一反馈生成 |
| MAGeometryUtils | 几何计算 (凸包、割草机航线等) |
| MASceneQuery | 场景语义查询 |

## 7. 通信协议 (MACommSubsystem)

通信子系统作为 UE5 与外部系统的接口，负责消息的收发和序列化。

### 7.1 消息信封格式

```json
{
    "message_type": "skill_list",
    "timestamp": 1703836800000,
    "message_id": "uuid-string",
    "payload": { ... }
}
```

### 7.2 消息类型

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

### 7.3 技能列表格式

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

### 7.4 反馈格式

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

### 7.5 配置选项

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


## 8. 数据流

### 8.1 指令执行流程

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

## 9. UI 系统架构

### 9.1 概述

本项目实现了 TopDown 视角下的指令输入界面，支持多种交互模式：

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           UI 系统架构                                        │
│                                                                             │
│  ┌─────────────────┐                                                        │
│  │ MAPlayerController │  ◄── Z 键切换 UI / M 键切换模式                      │
│  │ (Input 层)        │                                                      │
│  └────────┬─────────┘                                                       │
│           │                                                                 │
│           ▼                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MAHUD                                             │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐      │   │
│  │  │ SimpleMainWidget│  │ ModifyWidget    │  │ EditWidget      │      │   │
│  │  │ (主界面)        │  │ (标签编辑)      │  │ (场景编辑)      │      │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘      │   │
│  │  ┌─────────────────┐  ┌─────────────────┐                           │   │
│  │  │ EmergencyWidget │  │ TaskPlannerWidget│                          │   │
│  │  │ (突发事件)      │  │ (任务规划)      │                           │   │
│  │  └─────────────────┘  └─────────────────┘                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    MACommSubsystem                                   │   │
│  │  - SendUIInputMessage()                                              │   │
│  │  - SendButtonEventMessage()                                          │   │
│  │  - OnTaskPlanReceived 委托                                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 9.2 UI 组件说明

| 组件 | 功能 | 实现方式 |
|------|------|----------|
| **MASimpleMainWidget** | 主界面 UI (输入框 + 结果显示) | 纯 C++ 动态创建 |
| **MAEmergencyWidget** | 突发事件详情界面 (相机画面 + 操作按钮) | 纯 C++ 动态创建 |
| **MAModifyWidget** | Modify 模式修改面板 (Actor 标签编辑) | 纯 C++ 动态创建 |
| **MAEditWidget** | Edit 模式编辑面板 (JSON 编辑 + Goal/Zone 创建) | 纯 C++ 动态创建 |
| **MATaskPlannerWidget** | 任务规划工作台 (DAG 编辑器 + 指令输入) | 纯 C++ 动态创建 |
| **MADAGCanvasWidget** | DAG 画布 (拓扑排序布局 + 节点/边渲染) | 纯 C++ 动态创建 |
| **MAMiniMapWidget** | 小地图显示 | 纯 C++ 动态创建 |

### 9.3 鼠标模式

| 模式 | 功能 | 切换方式 |
|------|------|----------|
| Select | 框选/点选 Agent | M 键循环 |
| Deployment | 部署物品 | M 键循环 |
| Modify | 编辑 Actor 语义标签 | M 键循环 |
| Edit | 编辑临时场景图 | M 键循环 |


## 10. 场景图管理系统 (MASceneGraphManager)

### 10.1 概述

场景图管理系统用于管理场景中 Actor 的语义标注数据，支持单选和多选标注，并将数据持久化到 JSON 文件。

### 10.2 数据结构

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

### 10.3 API

```cpp
// 解析用户输入
bool ParseLabelInput(const FString& Input, FString& OutId, FString& OutType, FString& OutError);

// 添加场景节点
bool AddSceneNode(const FString& Input, const FVector& Location, AActor* Actor, FString& OutMessage);

// 获取所有节点 (用于可视化)
TArray<FMASceneGraphNode> GetAllNodes() const;

// 通过 GUID 查询所属节点
TArray<FMASceneGraphNode> FindNodesByGuid(const FString& Guid) const;

// 加载/保存场景图
void LoadSceneGraph();
void SaveSceneGraph();
```

## 11. Edit Mode 系统

### 11.1 概述

Edit Mode 用于模拟任务执行过程中"新情况"（动态变化）的交互模式。与 Modify Mode（持久化修改源场景图文件）不同，Edit Mode 的所有操作仅针对临时场景图文件进行，不影响源文件。

### 11.2 MAEditModeManager API

```cpp
// 临时场景图管理
bool CreateTempSceneGraph();           // 从源文件复制创建临时文件
void DeleteTempSceneGraph();           // 删除临时文件
bool IsEditModeAvailable() const;      // 检查 Edit Mode 是否可用

// POI 管理
AMAPointOfInterest* CreatePOI(const FVector& WorldLocation);
void DestroyPOI(AMAPointOfInterest* POI);
void DestroyAllPOIs();

// Node 操作
bool AddNode(const FString& NodeJson, FString& OutError);
bool DeleteNode(const FString& NodeId, FString& OutError);
bool EditNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError);
bool CreateGoal(const FVector& Location, const FString& Description, FString& OutError);
bool CreateZone(const TArray<FVector>& Vertices, const FString& Description, FString& OutError);
```

### 11.3 可视化 Actor

| Actor 类型 | 组件 | 外观 | 用途 |
|-----------|------|------|------|
| **AMAPointOfInterest** | UNiagaraComponent | 蓝色粒子效果 | 临时标记点 |
| **AMAGoalActor** | UStaticMeshComponent + UTextRenderComponent | 红色锥体 + 文本标签 | Goal 节点可视化 |
| **AMAZoneActor** | USplineComponent + USplineMeshComponent | 蓝色样条曲线边界 | Zone 节点可视化 |

### 11.4 场景变化消息类型

| 消息类型 | 枚举值 | Payload |
|---------|--------|---------|
| AddNode | add_node | 完整 Node JSON |
| DeleteNode | delete_node | Node ID |
| EditNode | edit_node | 修改后的 Node JSON |
| AddGoal | add_goal | Goal Node JSON |
| DeleteGoal | delete_goal | Goal ID |
| AddZone | add_zone | Zone Node JSON |
| DeleteZone | delete_zone | Zone ID |


## 12. 配置文件

配置文件位于 `config/` 目录：

| 文件 | 内容 |
|------|------|
| simulation.json | 服务器地址、轮询设置、观察者位置 |
| agents.json | 机器人类型定义、实例列表 |
| environment.json | 充电站、道具配置 |

### 12.1 agents.json 示例

```json
{
  "version": "1.0",
  "agent_types": {
    "UAV": "/Game/Blueprints/BP_MAUAVCharacter.BP_MAUAVCharacter_C",
    "UGV": "/Game/Blueprints/BP_MAUGVCharacter.BP_MAUGVCharacter_C",
    "Quadruped": "/Game/Blueprints/BP_MAQuadrupedCharacter.BP_MAQuadrupedCharacter_C",
    "Humanoid": "/Game/Blueprints/BP_MAHumanoidCharacter.BP_MAHumanoidCharacter_C"
  },
  "agents": [
    { "id": "UAV_0", "type": "UAV", "position": [1000, 0, 500] },
    { "id": "UGV_0", "type": "UGV", "position": [0, 0, 0] }
  ]
}
```

## 13. 扩展指南

### 13.1 添加新机器人类型

1. 创建新的 Character 类继承 `AMACharacter`
2. 在 `MAAgentManager::GetClassPathForType()` 添加类型映射
3. 在 `agents.json` 的 `agent_types` 中配置蓝图路径
4. 根据需要重写导航方法 (`TryNavigateTo`, `StopNavigation`)

### 13.2 添加新技能

1. 创建新的 GameplayAbility 类继承 `UMASkillBase`
2. 在 `MASkillTags` 中添加对应的 GameplayTag
3. 在 `MASkillComponent` 中添加激活/取消接口
4. 在 `MACommandManager` 中添加指令类型和 Tag 映射
5. 在 `MAFeedbackGenerator` 中添加反馈生成逻辑

### 13.3 添加新的通信消息类型

1. 在 `MACommTypes.h` 中定义新的消息结构
2. 在 `EMACommMessageType` 枚举中添加类型
3. 在 `MACommSubsystem::HandlePollResponse()` 中添加处理逻辑

## 14. StateTree + Skill 架构

### 14.1 架构概述

本项目采用 **StateTree + Skill** 架构：

```
StateTree (大脑 - 状态决策)          Skill (手脚 - 技能执行)
┌─────────────────────────┐         ┌─────────────────────────┐
│  Root                   │         │  Skills                 │
│  ├── Idle State         │◄───────►│  ├── SK_Navigate        │
│  ├── Navigate State     │  Tags   │  ├── SK_Search          │
│  ├── Search State       │◄───────►│  ├── SK_Follow          │
│  └── Charge State       │         │  └── SK_Place           │
└─────────────────────────┘         └─────────────────────────┘
```

- **StateTree**: 高层策略决策，管理 Character 的主模式
- **Skill**: 具体技能执行，处理导航、动画、反馈等
- **Gameplay Tags**: 两者之间的通信桥梁

### 14.2 StateTree Task

| Task | 功能 |
|------|------|
| `MASTTask_Navigate` | 导航到指定位置 |
| `MASTTask_Search` | 区域搜索 |
| `MASTTask_Follow` | 跟随目标 |
| `MASTTask_Charge` | 自动充电 |
| `MASTTask_TakeOff` | 起飞 |
| `MASTTask_Land` | 降落 |

### 14.3 StateTree Condition

| Condition | 功能 |
|-----------|------|
| `MASTCondition_HasCommand` | 检查是否收到指定命令 |
| `MASTCondition_LowEnergy` | 检查电量 < 阈值 |
| `MASTCondition_FullEnergy` | 检查电量 >= 阈值 |


## 15. Sensor Component 设计

### 15.1 Component 模式架构

```
UMASensorComponent (USceneComponent)
    └── UMACameraSensorComponent
        - 作为 Character 的组件
        - 生命周期由 Character 管理
        - 通过 Character API 操作
```

### 15.2 MACharacter Sensor 管理 API

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
```

### 15.3 MACameraSensorComponent 功能

| 方法 | 按键 | 说明 |
|------|------|------|
| `TakePhoto(FilePath)` | L | 拍照保存到 Saved/Screenshots/ |
| `StartRecording(FPS)` | R | 开始录像（序列帧） |
| `StopRecording()` | R | 停止录像 |
| `StartTCPStream(Port, FPS)` | V | 启动 TCP 视频流 |
| `StopTCPStream()` | V | 停止 TCP 视频流 |

## 16. 碰撞系统

### 16.1 Agent 间碰撞策略

项目统一使用 **CapsuleComponent** 实现 Agent 间碰撞：

- CapsuleComponent (ACharacter 默认): 用于地面检测、移动系统、Agent 间碰撞
- SkeletalMesh: 仅用于渲染，CollisionEnabled: NoCollision

### 16.2 配置方式

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

## 17. ViewportManager 与 Direct Control

ViewportManager 负责管理相机视角切换和 Agent View Mode (Direct Control 模式)。

### 17.1 核心功能

- 视角切换：Tab 键在不同 Agent 相机之间切换
- Agent View Mode：进入 Agent 视角后可直接控制该 Agent
- Direct Control：WASD 移动、鼠标/方向键旋转视角

### 17.2 ViewportManager 属性

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| bIsInAgentViewMode | bool | false | 是否处于 Agent 视角模式 |
| ControlledAgent | TWeakObjectPtr | null | 当前控制的 Agent |
| LookSensitivityYaw | float | 14.0 | 水平旋转灵敏度 |
| LookSensitivityPitch | float | 14.0 | 垂直俯仰灵敏度 |

### 17.3 Direct Control 特性

- 进入 Direct Control 后，Agent 的 `bOrientRotationToMovement` 被禁用
- RTS 命令不会影响处于 Direct Control 的 Agent
- Drone 支持 Space/Ctrl 垂直移动
- 方向键灵敏度为鼠标的 0.125 倍，适合精细调整

## 18. Squad 系统

参考 Company of Heroes 的 Squad 系统，Squad 是一组 Agent 的组合。

### 18.1 Squad 管理 API

```cpp
// 创建 Squad
UMASquad* Squad = SquadManager->CreateSquad(Members, Leader, "MySquad");

// 解散 Squad
SquadManager->DisbandSquad(Squad);

// 查询
UMASquad* Squad = SquadManager->GetSquadByAgent(Agent);
```

### 18.2 编队类型 (EMAFormationType)

| 类型 | 说明 |
|------|------|
| None | 无编队 |
| Line | 横向一字排开 |
| Column | 纵队 |
| Wedge | V形楔形 |
| Diamond | 菱形 |
| Circle | 圆形 |

---

按键说明详见 [KEYBINDINGS.md](KEYBINDINGS.md)
