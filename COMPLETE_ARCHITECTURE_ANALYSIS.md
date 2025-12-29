# UE5 多智能体仿真项目 - 完整架构分析

## 项目概述

这是一个基于 Unreal Engine 5 的多智能体仿真框架，支持异构智能体（无人机、无人车、四足机器人、人形机器人）的协同仿真。系统采用**三层架构**设计，实现了从外部规划器到仿真执行的完整流程。

---

## 核心架构设计

### 三层架构模型

```
┌─────────────────────────────────────────────────────────────┐
│  指令调度层 (MACommandManager)                              │
│  - 接收 Python 端技能列表                                   │
│  - 参数预处理（场景查询）                                   │
│  - 设置 StateTree Tag 触发技能                              │
│  - 收集反馈并上报                                           │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│  技能管理层 (MASkillComponent + StateTree)                  │
│  - 技能参数管理                                             │
│  - 技能激活/取消                                            │
│  - 反馈上下文收集                                           │
│  - 能量系统管理                                             │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│  技能内核层 (SK_Navigate, SK_Search, SK_Follow 等)          │
│  - 具体技能实现                                             │
│  - 运动控制                                                 │
│  - 传感器模拟                                               │
│  - 结果反馈                                                 │
└─────────────────────────────────────────────────────────────┘
```

### 数据流向

```
Python 规划器
    ↓ (HTTP POST: skill_list)
MACommSubsystem (通信子系统)
    ↓ (轮询 /api/sim/poll)
MACommandManager (指令调度)
    ↓ (设置 Tag)
MASkillComponent (技能管理)
    ↓ (激活技能)
SK_Navigate/SK_Search/... (技能实现)
    ↓ (执行完成)
MASkillComponent (收集反馈)
    ↓ (HTTP POST: feedback)
MACommSubsystem
    ↓ (发送到 Python)
Python 规划器
```

---

## 核心子系统详解

### 1. 配置管理系统 (Core/Config/)

**职责**: 统一加载和管理所有配置文件

**关键类**: `UMAConfigManager` (GameInstanceSubsystem)

**配置文件**:
- `config/simulation.json` - 仿真参数（服务器、观察者位置、生成设置）
- `config/agents.json` - 机器人配置（类型定义、实例列表）
- `config/environment.json` - 环境配置（地图、充电站、物体）

**主要功能**:
```cpp
// 加载所有配置
bool LoadAllConfigs();

// 访问配置数据
UPROPERTY() TArray<FMAAgentConfigData> AgentConfigs;
UPROPERTY() TArray<FMAChargingStationConfig> ChargingStations;
UPROPERTY() FString PlannerServerURL;
UPROPERTY() bool bUseStateTree;
```

**配置数据结构**:
```json
{
  "agents": [
    {
      "type": "UAV",
      "count": 2,
      "instances": [
        {"name": "UAV_01", "position": [0, 500, 0]}
      ]
    }
  ]
}
```

---

### 2. 游戏流程系统 (Core/GameFlow/)

**职责**: 管理游戏实例、地图加载、初始化流程

**关键类**:
- `UMAGameInstance` - 游戏实例，提供全局访问
- `AMAGameMode` - 仿真地图游戏模式
- `AMASetupGameMode` - 设置地图游戏模式

**初始化流程**:
```
UMAGameInstance::Init()
    ↓
UMAConfigManager::Initialize() - 加载配置
    ↓
UMACommSubsystem::Initialize() - 启动通信
    ↓
UMAAgentManager::Initialize() - 生成 Agent
    ↓
UMACommandManager::Initialize() - 初始化指令系统
```

---

### 3. Agent 管理系统 (Core/Manager/MAAgentManager)

**职责**: Agent 生命周期管理、生成、查询

**关键功能**:
```cpp
// 从配置生成所有 Agent
void SpawnAgentsFromConfig();

// 按类型查询
TArray<AMACharacter*> GetAgentsByType(EMAAgentType Type);

// 按 ID 查询
AMACharacter* GetAgentByID(const FString& AgentID);

// 委托
UPROPERTY() FOnAgentSpawned OnAgentSpawned;
UPROPERTY() FOnAgentDestroyed OnAgentDestroyed;
```

**支持的 Agent 类型**:
| 类型 | 类名 | 技能 |
|------|------|------|
| UAV | MAUAVCharacter | Navigate, Search, Follow, Charge, TakeOff, Land |
| FixedWingUAV | MAFixedWingUAVCharacter | Navigate, Search, Charge |
| UGV | MAUGVCharacter | Navigate, Carry, Charge |
| Quadruped | MAQuadrupedCharacter | Navigate, Search, Follow, Charge |
| Humanoid | MAHumanoidCharacter | Navigate, Place, Charge |

---

### 4. 指令调度系统 (Core/Manager/MACommandManager)

**职责**: 接收 Python 端指令，分发给 Agent，收集反馈

**执行模式**: 根据 `config/simulation.json` 中的 `use_state_tree` 配置自动选择：
- **StateTree 模式** (`use_state_tree: true`): 设置 GameplayTag，由 StateTree 检测并激活技能
- **直接激活模式** (`use_state_tree: false`): 直接调用 SkillComponent 的技能激活方法

**关键接口**:
```cpp
// 执行完整技能列表（异步，按时间步顺序）
void ExecuteSkillList(const FMASkillListMessage& SkillList);

// 发送单个指令
void SendCommand(AMACharacter* Agent, EMACommand Command);

// 完成委托
UPROPERTY() FOnSkillListCompleted OnSkillListCompleted;
```

**执行流程**:
```
ExecuteSkillList(SkillList)
    ↓
for each TimeStep:
    ↓
    for each Agent in TimeStep:
        ↓
        SendCommandToAgent(Agent, Command)
            ↓
            设置 StateTree Tag (Command.Navigate)
            ↓
            StateTree 检测 Tag 变化
            ↓
            激活对应技能
            ↓
            技能执行完成
            ↓
            清除 Tag
            ↓
            收集反馈
    ↓
    等待所有技能完成
    ↓
    发送时间步反馈到 Python
```

**指令类型**:
```cpp
enum class EMACommand : uint8
{
    Navigate, Follow, Charge, Search, Place,
    TakeOff, Land, ReturnHome, Idle
};
```

---

### 5. 通信系统 (Core/Comm/)

**职责**: 与 Python 规划器的 HTTP 通信

**关键类**: `UMACommSubsystem` (GameInstanceSubsystem)

**通信协议**:

#### 消息信封格式
```json
{
  "message_type": "skill_list",
  "timestamp": 1234567890000,
  "message_id": "uuid",
  "payload": {...}
}
```

#### 消息类型
- **UIInput** - UI 输入消息
- **ButtonEvent** - 按钮事件
- **TaskFeedback** - 任务反馈
- **TaskPlanDAG** - 任务规划 DAG
- **WorldModelGraph** - 世界模型图
- **SkillList** - 技能列表
- **QueryRequest** - 查询请求

#### 轮询机制
```
UE5 端定时轮询 Python 端 /api/sim/poll
    ↓
Python 返回待执行的技能列表
    ↓
UE5 执行技能列表
    ↓
执行完成后 POST 反馈到 /api/sim/message
```

**重试机制**:
- 指数退避: 1s, 2s, 4s
- 最多重试 3 次
- 4xx 错误不重试，5xx 错误重试

---

### 6. Agent 角色系统 (Agent/Character/)

**基类**: `AMACharacter` (Character + IAbilitySystemInterface)

**关键属性**:
```cpp
UPROPERTY() FString AgentID;
UPROPERTY() EMAAgentType AgentType;
UPROPERTY() bool bIsMoving;
UPROPERTY() FVector InitialLocation;  // 用于返航
UPROPERTY() UMASkillComponent* SkillComponent;
```

**关键接口**:
```cpp
// 技能接口
bool TryNavigateTo(FVector Destination);
bool TryFollowActor(AMACharacter* TargetActor, float FollowDistance);
void CancelNavigation();

// 技能集查询
bool HasSkill(EMASkillType Skill);
const TArray<EMASkillType>& GetAvailableSkills();

// 直接控制
void SetDirectControl(bool bEnabled);
void ApplyDirectMovement(FVector WorldDirection);
```

**子类实现**:
- `MAUAVCharacter` - 多旋翼无人机（飞行）
- `MAFixedWingUAVCharacter` - 固定翼无人机（飞行）
- `MAUGVCharacter` - 无人车（地面）
- `MAQuadrupedCharacter` - 四足机器人（地面）
- `MAHumanoidCharacter` - 人形机器人（地面）

---

### 7. 技能系统 (Agent/Skill/)

#### 7.1 技能管理层 (MASkillComponent)

**职责**: 单个 Agent 的技能管理

**继承**: `UAbilitySystemComponent` (GameplayAbilities)

**关键功能**:
```cpp
// 技能激活接口
bool TryActivateNavigate(FVector TargetLocation);
bool TryActivateSearch();
bool TryActivateFollow(AMACharacter* TargetCharacter);
bool TryActivatePlace();
bool TryActivateTakeOff();
bool TryActivateLand();
bool TryActivateReturnHome();

// 参数管理
const FMASkillParams& GetSkillParams();
FMASkillParams& GetSkillParamsMutable();

// 反馈上下文
const FMAFeedbackContext& GetFeedbackContext();
void UpdateFeedback(int32 Current, int32 Total);
void AddFoundObject(const FString& ObjectName, const FVector& Location, const TMap<FString, FString>& Attributes);

// 能量系统
void DrainEnergy(float DeltaTime);
void RestoreEnergy(float Amount);
float GetEnergyPercent();
bool IsLowEnergy();

// 完成通知
void NotifySkillCompleted(bool bSuccess, const FString& Message);
UPROPERTY() FOnSkillCompleted OnSkillCompleted;
```

**技能参数结构**:
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
    TArray<FVector> SearchBoundary;
    FMASemanticTarget SearchTarget;
    float SearchScanWidth;
    
    // Place
    FMASemanticTarget PlaceObject1;
    FMASemanticTarget PlaceObject2;
    
    // TakeOff/Land/ReturnHome
    float TakeOffHeight;
    float LandHeight;
    FVector HomeLocation;
};
```

#### 7.2 技能实现层 (Agent/Skill/Impl/)

**关键技能**:

##### SK_Navigate - 导航技能
```cpp
// 飞行器: 直接设置目标位置，定时检查到达
// 地面机器人: 使用 AIController 导航，投影到 NavMesh

void StartNavigation();
void CheckFlightArrival();  // 飞行器检查
void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);  // 地面机器人
```

**执行流程**:
1. 获取目标位置
2. 飞行器: 设置目标，定时检查距离
3. 地面机器人: 投影到 NavMesh，使用 AIController 导航
4. 到达目标或失败时通知完成

##### SK_Search - 搜索技能
```cpp
// 生成割草机航线（Lawnmower Pattern）
// 平滑移动到每个航点
// 收集发现的对象

void GenerateSearchPath();
void UpdateSearch();  // 定时更新位置
```

**执行流程**:
1. 获取搜索边界
2. 生成割草机航线
3. 定时移动到下一航点
4. 检测搜索区域内的对象
5. 完成后返回发现的对象列表

##### SK_Follow - 跟随技能
```cpp
// 跟随目标 Actor
// 保持指定距离
// 目标消失时失败
```

##### SK_Place - 放置技能
```cpp
// 放置两个对象
// 语义匹配
// 位置验证
```

##### SK_TakeOff/SK_Land - 起降技能
```cpp
// 起飞: 上升到指定高度
// 着陆: 下降到地面
```

##### SK_ReturnHome - 返航技能
```cpp
// 导航回初始位置
// 着陆
```

---

### 8. StateTree 系统 (Agent/StateTree/)

**职责**: 使用 StateTree 驱动技能执行

**配置控制**: 通过 `config/simulation.json` 中的 `use_state_tree` 参数控制：
- `true` - 启用 StateTree 模式，技能由 StateTree 状态机驱动
- `false` - 禁用 StateTree 模式，技能由 MACommandManager 直接激活

**关键组件**: `UMAStateTreeComponent` (继承自 UStateTreeComponent)
- `BeginPlay()` 时自动读取配置，决定是否启用
- `IsStateTreeEnabled()` - 检查是否启用
- `DisableStateTree()` / `EnableStateTree()` - 手动控制

**关键任务**:
- `MASTTask_Navigate` - 导航任务
- `MASTTask_Search` - 搜索任务
- `MASTTask_Follow` - 跟随任务
- `MASTTask_Place` - 放置任务
- `MASTTask_TakeOff` - 起飞任务
- `MASTTask_Land` - 着陆任务
- `MASTTask_ReturnHome` - 返航任务

**任务实现模式**:
```cpp
EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
    // 1. 获取 Agent 和 SkillComponent
    // 2. 激活对应技能
    // 3. 返回 Running
    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime)
{
    // 1. 检查命令 Tag 是否还存在
    // 2. Tag 清除表示技能完成
    // 3. 返回 Succeeded
    return EStateTreeRunStatus::Succeeded;
}

void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
    // 如果被中断，取消技能
}
```

**Tag 驱动机制**:
```
CommandManager 设置 Tag (Command.Navigate)
    ↓
StateTree 检测 Tag 变化
    ↓
EnterState() 激活技能
    ↓
Tick() 检查 Tag 是否清除
    ↓
技能完成时清除 Tag
    ↓
Tick() 返回 Succeeded
    ↓
StateTree 转移到下一状态
```

---

### 9. 世界查询系统 (Utils/MAWorldQuery)

**职责**: 查询 UE5 世界状态

**关键接口**:
```cpp
// 获取所有实体
static TArray<FMAEntityNode> GetAllEntities(UWorld* World);

// 获取所有机器人
static TArray<FMAEntityNode> GetAllRobots(UWorld* World);

// 获取所有道具
static TArray<FMAEntityNode> GetAllProps(UWorld* World);

// 获取边界特征
static TArray<FMABoundaryFeature> GetBoundaryFeatures(UWorld* World, const TArray<FString>& Categories);

// 根据 ID 获取实体
static FMAEntityNode GetEntityById(UWorld* World, const FString& EntityId);

// 获取完整世界状态
static FMAWorldQueryResult GetWorldState(UWorld* World);
```

**实体节点结构**:
```cpp
struct FMAEntityNode
{
    FString Id;                          // 实体 ID
    FString Category;                    // robot, building, prop, area
    FString Type;                        // UAV, UGV, house, road
    TMap<FString, FString> Properties;   // 属性键值对
};
```

**边界特征结构**:
```cpp
struct FMABoundaryFeature
{
    FString Label;           // 实体标签
    FString Kind;            // area, line, point, rectangle, circle
    TArray<FVector2D> Coords;  // 坐标
    FVector2D Center;        // 圆心
    float Radius;            // 半径
};
```

---

## Python 集成

### 1. 技能列表发送脚本 (scripts/send_skill_list.py)

**功能**: 发送技能列表到 UE5 并接收反馈

**技能列表格式**:
```json
{
  "0": {
    "UAV_01": {
      "skill": "take_off",
      "params": {}
    }
  },
  "1": {
    "UAV_01": {
      "skill": "navigate",
      "params": {
        "dest": {"x": 1000, "y": 500, "z": 1000},
        "target_entity": "SM_FireHydrant"
      }
    }
  },
  "2": {
    "UAV_01": {
      "skill": "search",
      "params": {
        "area": {
          "kind": "area",
          "coords": [[x1, y1], [x2, y2], ...]
        },
        "target": {
          "class": "object",
          "type": "any"
        }
      }
    }
  }
}
```

**使用方法**:
```bash
# 启动服务器
python scripts/send_skill_list.py --server --port 8080

# 打印技能列表
python scripts/send_skill_list.py --print
```

### 2. 世界查询脚本 (scripts/world_query.py)

**功能**: 查询 UE5 世界状态

**查询类型**:
- `world_state` - 完整世界状态
- `all_entities` - 所有实体
- `all_robots` - 所有机器人
- `all_props` - 所有道具
- `boundaries` - 边界特征
- `entity_by_id` - 根据 ID 查询

**使用方法**:
```bash
# 启动服务器
python scripts/world_query.py --server --port 8080

# 查询所有 API 并保存
python scripts/world_query.py --server --query-all --output datasets

# 交互式查询
python scripts/world_query.py --server
> world_state
> all_robots
> boundaries
> entity <id>
```

---

## 数据流示例

### 完整执行流程

```
1. Python 规划器生成技能列表
   {
     "0": {"UAV_01": {"skill": "take_off", "params": {}}},
     "1": {"UAV_01": {"skill": "navigate", "params": {"dest": {...}}}}
   }

2. Python 发送 HTTP POST 到 /api/sim/message
   Message: {
     "message_type": "skill_list",
     "payload": {...}
   }

3. UE5 轮询 /api/sim/poll 获取消息
   MACommSubsystem::PollForMessages()
   ↓
   HandlePollResponse()
   ↓
   MACommandManager::ExecuteSkillList()

4. CommandManager 执行时间步 0
   for each Agent in TimeStep 0:
     SendCommandToAgent(UAV_01, TakeOff)
     ↓
     设置 Tag: Command.TakeOff
     ↓
     StateTree 检测 Tag
     ↓
     MASTTask_TakeOff::EnterState()
     ↓
     SkillComponent::TryActivateTakeOff()
     ↓
     SK_TakeOff::ActivateAbility()
     ↓
     执行起飞逻辑
     ↓
     完成时清除 Tag
     ↓
     SkillComponent::NotifySkillCompleted()
     ↓
     收集反馈

5. 所有技能完成后发送反馈
   FMATimeStepFeedback {
     TimeStep: 0,
     Feedbacks: [
       {
         AgentId: "UAV_01",
         SkillName: "take_off",
         bSuccess: true,
         Message: "Take off executed successfully"
       }
     ]
   }

6. MACommSubsystem 发送 HTTP POST 到 /api/sim/message
   Message: {
     "message_type": "task_feedback",
     "payload": {
       "time_step": 0,
       "feedbacks": [...]
     }
   }

7. Python 接收反馈并继续规划
```

---

## 关键设计模式

### 1. 三层架构分离

- **指令调度层**: 负责接收、解析、分发指令
- **技能管理层**: 负责参数管理、反馈收集、能量管理
- **技能内核层**: 负责具体执行逻辑

**优点**: 清晰的职责分离，易于扩展新技能

### 2. Tag 驱动的 StateTree

- 使用 GameplayTag 作为状态转移的触发器
- StateTree 任务通过检查 Tag 的存在/清除来判断技能完成
- 避免了复杂的回调链

**优点**: 解耦 StateTree 和技能实现，支持异步执行

### 3. 异步时间步执行

- 每个时间步内的所有技能并行执行
- 等待所有技能完成后再进入下一时间步
- 支持技能之间的依赖关系

**优点**: 高效的并行执行，支持复杂的任务序列

### 4. 参数预处理

- CommandManager 在分发指令前进行场景查询
- 将语义参数（如 target_entity）转换为具体坐标
- 支持智能参数调整

**优点**: 规划器可以使用高级语义参数，UE5 端自动处理细节

### 5. 反馈上下文

- SkillComponent 维护反馈上下文
- 技能执行过程中更新反馈信息
- 完成时一次性收集所有反馈

**优点**: 完整的执行信息，支持详细的反馈分析

---

## 扩展点

### 1. 添加新技能

1. 在 `Agent/Skill/Impl/` 创建 `SK_NewSkill.h/cpp`
2. 继承 `UGameplayAbility`
3. 实现 `ActivateAbility()`, `EndAbility()` 等方法
4. 在 `MASkillComponent` 中注册技能
5. 在 `MACharacter` 子类中添加到 `AvailableSkills`
6. 创建对应的 StateTree 任务

### 2. 添加新 Agent 类型

1. 在 `Agent/Character/` 创建 `MANewCharacter.h/cpp`
2. 继承 `AMACharacter`
3. 在 `InitializeSkillSet()` 中定义可用技能
4. 在 `MAAgentManager::GetClassPathForType()` 中注册类路径
5. 在 `config/agents.json` 中添加配置

### 3. 添加新查询类型

1. 在 `MAWorldQuery` 中添加新的静态方法
2. 在 `MACommSubsystem::HandleQueryRequest()` 中处理新查询类型
3. 在 Python 脚本中调用新查询

---

## 性能考虑

### 1. 技能执行

- 飞行器导航: 定时检查（0.1s 间隔）
- 地面机器人导航: 使用 AIController 的路径跟随
- 搜索: 平滑移动（0.05s 更新间隔）

### 2. 通信

- 轮询间隔: 1.0s（可配置）
- HTTP 超时: 10s（发送消息）, 5s（轮询）
- 重试机制: 指数退避，最多 3 次

### 3. 内存

- Agent 对象池: 预生成所有 Agent
- 技能参数缓存: 每个 Agent 一份
- 反馈上下文: 执行完成后清空

---

## 调试技巧

### 1. 日志

```cpp
// 启用详细日志
UE_LOG(LogMACommSubsystem, Verbose, TEXT("..."));
UE_LOG(LogMACommandManager, Log, TEXT("..."));
```

### 2. 模拟模式

```json
{
  "server": {
    "use_mock_data": true
  }
}
```

### 3. 世界查询

```bash
python scripts/world_query.py --server --query-all
```

### 4. 技能列表打印

```bash
python scripts/send_skill_list.py --print
```

---

## 总结

这个项目采用了**模块化、分层、异步**的设计理念：

1. **模块化**: 清晰的模块划分，各模块职责明确
2. **分层**: 三层架构实现了关注点分离
3. **异步**: 支持并行执行和异步反馈
4. **可扩展**: 易于添加新技能、新 Agent 类型、新查询方式
5. **可集成**: 通过 HTTP 协议与外部规划器集成

这样的设计使得系统既能支持复杂的多智能体协同任务，又能保持代码的可维护性和可扩展性。
