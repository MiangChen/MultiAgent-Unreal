# 多机器人仿真平台架构文档

## 第一部分：文字描述

### 1. 平台概述

本平台是一个基于 UE5 的多机器人仿真系统，用于支持多机器人任务规划的研究与验证。

**平台输入**：外部规划器（Python 端）发送的技能列表，每个技能包含执行机器人、技能类型、技能参数。

**平台输出**：技能执行反馈，包含执行结果（成功/失败）、执行消息、发现的目标信息等结构化数据。

平台内部采用三层架构设计：指令调度层、技能管理层、技能内核层。此外还包括通信接口和 UI 交互界面。

---

### 2. 三层架构

#### 2.1 第一层：指令调度层（MACommandManager）

**职责**：接收外部技能列表，按时间步调度执行，收集并汇总反馈。

**输入**：
- 技能列表消息（FMASkillListMessage），按时间步组织，每个时间步包含多个机器人的技能指令

**输出**：
- 时间步反馈（FMATimeStepFeedback），包含该时间步所有机器人的执行结果

**工作流程**：
1. 接收 Python 端发送的技能列表
2. 按时间步顺序执行：先执行时间步 0 的所有指令，等待全部完成后再执行时间步 1
3. 对每个指令，调用参数处理器进行参数预处理，然后分发给对应机器人的技能管理层
4. 等待当前时间步所有技能完成，收集反馈
5. 将反馈发送给 Python 端，继续执行下一个时间步

---

#### 2.2 第二层：技能管理层（MASkillComponent + MASkillParamsProcessor + MAFeedbackGenerator）

**职责**：管理单个机器人的技能参数、触发技能模拟、生成执行反馈。

技能管理层分为三个环节：

##### 2.2.1 参数处理环节（MASkillParamsProcessor）

**输入**：
- 原始技能参数（如边界坐标列表、目标语义标签 JSON）

**输出**：
- 处理后的技能参数（FMASkillParams）
- 反馈上下文初始数据（FMAFeedbackContext）

**处理逻辑**（以 Search 技能为例）：

1. **边界信息处理**：如果输入的边界信息已经是坐标列表形式，直接存储到 `SearchBoundary`，不做额外处理
2. **目标语义解析**：解析目标 JSON（如 `{"class": "object", "type": "boat", "features": {"color": "black"}}`），提取 class、type、features 字段
3. **场景图查询**：根据语义标签在 UE5 场景图中查找匹配的对象
   - 如果找到匹配对象，获取其位置、状态等信息
   - 判断目标是否在搜索边界范围内
   - 将找到的对象信息存入反馈上下文（FoundObjects、FoundLocations）
4. **回退机制**：如果场景图查询未找到，回退到 UE5 原生场景查询

##### 2.2.2 技能模拟环节（StateTree + GameplayAbility）

**输入**：
- 处理后的技能参数（FMASkillParams）

**输出**：
- 技能执行结果（成功/失败）
- 执行过程中更新的反馈上下文

**执行逻辑**（以 Search 技能为例）：
1. StateTree 检测到 Search 命令标签，进入 Search 状态
2. 激活 SK_Search 技能（GameplayAbility）
3. 根据边界坐标规划搜索航线（如割草机式航线）
4. 调用导航技能沿航线飞行
5. 执行完成后，调用 `NotifySkillCompleted(bSuccess, Message)` 通知完成

##### 2.2.3 反馈生成环节（MAFeedbackGenerator）

**输入**：
- 技能执行结果（成功/失败）
- 反馈上下文（FMAFeedbackContext）

**输出**：
- 结构化反馈（FMASkillExecutionFeedback），包含：
  - agent_id：执行机器人 ID
  - skill_name：技能名称
  - success：是否成功
  - message：执行消息（如"成功执行搜索技能，发现 black sailboat"或"成功执行搜索技能，未发现目标"）
  - data：额外数据（发现的对象列表、位置等）

---

#### 2.3 第三层：技能内核层（GameplayAbility）

**职责**：实现具体的技能执行逻辑。

**输入**：
- 技能参数（从 MASkillComponent 获取）

**输出**：
- 技能执行结果
- 执行过程中的状态更新

**技能实现**：
| 技能 | 类名 | 核心逻辑 |
|------|------|----------|
| Navigate | SK_Navigate | 点对点导航，地面机器人用 NavMesh，飞行器直线飞行 |
| Search | SK_Search | 根据边界生成割草机航线，沿航线导航 |
| Follow | SK_Follow | 实时追踪目标，保持指定距离 |
| Place | SK_Place | 拾取物体、导航到目标位置、放置物体 |
| Charge | SK_Charge | 导航到充电站、等待充电完成 |
| TakeOff | SK_TakeOff | 垂直上升到指定高度 |
| Land | SK_Land | 垂直下降到地面 |
| ReturnHome | SK_ReturnHome | 导航回出生点 |

---

### 3. 通信接口（MACommSubsystem）

**职责**：作为 UE5 仿真端与外部系统的通信桥梁。

**入站消息**（Python → UE5）：
- 技能列表（SkillList）：包含按时间步组织的技能指令
- 查询请求（QueryRequest）：世界状态查询

**出站消息**（UE5 → Python）：
- 时间步反馈（TimeStepFeedback）：每个时间步执行完成后发送
- 世界状态响应（WorldState）：响应查询请求

**通信方式**：
- HTTP 轮询：UE5 定期向 Python 端轮询消息
- HTTP POST：UE5 主动发送反馈消息

---

### 4. UI 交互界面

**职责**：提供用户操作入口，显示仿真状态。

**主要组件**：
- MASimpleMainWidget：主界面，包含指令输入框和结果显示
- MAEmergencyWidget：突发事件处理界面
- MAEditWidget：场景编辑界面
- MAMiniMapWidget：小地图显示

**交互流程**：
1. 用户在输入框输入自然语言指令
2. 指令通过通信接口发送到 Python 端
3. Python 端返回技能列表
4. 技能列表进入三层架构执行
5. 执行结果显示在界面上

---

## 第二部分：类视角架构图

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              外部系统 (Python 端)                                    │
│                                                                                     │
│   输入: 自然语言指令 ──► 任务规划器 ──► 技能列表 (JSON)                              │
│   输出: 技能执行反馈 (JSON)                                                          │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        │ HTTP (JSON)
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              通信接口层                                              │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                         UMACommSubsystem                                       │  │
│  │  - SendTimeStepFeedback()      // 发送时间步反馈                               │  │
│  │  - OnSkillAllocationReceived   // 接收技能分配委托                             │  │
│  │  - HandlePollResponse()        // 处理轮询响应                                 │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        │ FMASkillListMessage
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         第一层: 指令调度层                                           │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                         UMACommandManager                                      │  │
│  │                                                                               │  │
│  │  输入: FMASkillListMessage (技能列表)                                          │  │
│  │  输出: FMATimeStepFeedback (时间步反馈)                                        │  │
│  │                                                                               │  │
│  │  - ExecuteSkillList()          // 执行技能列表                                 │  │
│  │  - ExecuteCurrentTimeStep()    // 执行当前时间步                               │  │
│  │  - OnSkillCompleted()          // 技能完成回调                                 │  │
│  │  - SendCommandToAgent()        // 分发指令到 Agent                             │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        │ EMACommand + FMAAgentSkillCommand
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         第二层: 技能管理层                                           │
│                                                                                     │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │                    参数处理: FMASkillParamsProcessor                         │    │
│  │                                                                             │    │
│  │  输入: 原始参数 (边界坐标、目标语义 JSON)                                     │    │
│  │  输出: FMASkillParams + FMAFeedbackContext                                  │    │
│  │                                                                             │    │
│  │  - Process()                   // 主处理入口                                 │    │
│  │  - ProcessSearch()             // Search 参数处理                            │    │
│  │  - ProcessNavigate()           // Navigate 参数处理                          │    │
│  │  - ProcessFollow()             // Follow 参数处理                            │    │
│  │  - ProcessPlace()              // Place 参数处理                             │    │
│  │                                                                             │    │
│  │  场景图查询: UMASceneGraphManager + FMASceneGraphQuery                       │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                        │                                            │
│                                        ▼                                            │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │                    技能模拟: UMASkillComponent                               │    │
│  │                                                                             │    │
│  │  输入: FMASkillParams (处理后的参数)                                         │    │
│  │  输出: 技能执行结果 (bSuccess, Message)                                      │    │
│  │                                                                             │    │
│  │  - TryActivateNavigate()       // 激活导航技能                               │    │
│  │  - TryActivateSearch()         // 激活搜索技能                               │    │
│  │  - TryActivateFollow()         // 激活跟随技能                               │    │
│  │  - TryActivatePlace()          // 激活放置技能                               │    │
│  │  - NotifySkillCompleted()      // 通知技能完成                               │    │
│  │                                                                             │    │
│  │  数据存储:                                                                   │    │
│  │  - FMASkillParams SkillParams          // 技能参数                           │    │
│  │  - FMAFeedbackContext FeedbackContext  // 反馈上下文                         │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                        │                                            │
│                                        ▼                                            │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │                    反馈生成: FMAFeedbackGenerator                            │    │
│  │                                                                             │    │
│  │  输入: Agent, Command, bSuccess, FMAFeedbackContext                         │    │
│  │  输出: FMASkillExecutionFeedback                                            │    │
│  │                                                                             │    │
│  │  - Generate()                  // 统一生成入口                               │    │
│  │  - GenerateSearchFeedback()    // Search 反馈生成                            │    │
│  │  - GenerateNavigateFeedback()  // Navigate 反馈生成                          │    │
│  │  - GenerateFollowFeedback()    // Follow 反馈生成                            │    │
│  │  - GeneratePlaceFeedback()     // Place 反馈生成                             │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        │ GameplayTag 触发
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         第三层: 技能内核层                                           │
│                                                                                     │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │                    UMASkillBase (GameplayAbility 基类)                       │    │
│  │                                                                             │    │
│  │  输入: FMASkillParams (从 SkillComponent 获取)                               │    │
│  │  输出: 技能执行结果                                                          │    │
│  │                                                                             │    │
│  │  - ActivateAbility()           // 技能激活                                   │    │
│  │  - EndAbility()                // 技能结束                                   │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                        │                                            │
│                    ┌───────────────────┼───────────────────┐                        │
│                    ▼                   ▼                   ▼                        │
│  ┌──────────────────────┐ ┌──────────────────────┐ ┌──────────────────────┐         │
│  │    SK_Navigate       │ │    SK_Search         │ │    SK_Follow         │         │
│  │                      │ │                      │ │                      │         │
│  │ - 点对点导航         │ │ - 生成割草机航线     │ │ - 实时追踪目标       │         │
│  │ - NavMesh/直线飞行   │ │ - 沿航线导航         │ │ - 保持跟随距离       │         │
│  └──────────────────────┘ └──────────────────────┘ └──────────────────────┘         │
│                                                                                     │
│  ┌──────────────────────┐ ┌──────────────────────┐ ┌──────────────────────┐         │
│  │    SK_Place          │ │    SK_Charge         │ │    SK_TakeOff/Land   │         │
│  │                      │ │                      │ │                      │         │
│  │ - 拾取物体           │ │ - 导航到充电站       │ │ - 垂直升降           │         │
│  │ - 导航到目标位置     │ │ - 等待充电完成       │ │ - 高度控制           │         │
│  │ - 放置物体           │ │                      │ │                      │         │
│  └──────────────────────┘ └──────────────────────┘ └──────────────────────┘         │
└─────────────────────────────────────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              UI 交互界面                                             │
│                                                                                     │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                              MAHUD                                             │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐                │  │
│  │  │ SimpleMainWidget│  │ EmergencyWidget │  │ EditWidget      │                │  │
│  │  │ (主界面)        │  │ (突发事件)      │  │ (场景编辑)      │                │  │
│  │  │                 │  │                 │  │                 │                │  │
│  │  │ - 指令输入框    │  │ - 事件详情      │  │ - JSON 编辑     │                │  │
│  │  │ - 结果显示      │  │ - 操作按钮      │  │ - 节点管理      │                │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘                │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 附录：核心数据结构

### 技能参数 (FMASkillParams)
```cpp
struct FMASkillParams
{
    // Navigate
    FVector TargetLocation;
    float AcceptanceRadius;
    
    // Search
    TArray<FVector> SearchBoundary;      // 搜索区域边界坐标
    FMASemanticTarget SearchTarget;       // 搜索目标语义
    
    // Follow
    TWeakObjectPtr<AMACharacter> FollowTarget;
    float FollowDistance;
    
    // Place
    FMASemanticTarget PlaceObject1;       // 被放置物体
    FMASemanticTarget PlaceObject2;       // 放置位置
};
```

### 语义标签 (FMASemanticTarget)
```cpp
struct FMASemanticTarget
{
    FString Class;                        // object, robot, ground
    FString Type;                         // boat, box, UGV
    TMap<FString, FString> Features;      // {"color": "black", "subtype": "sailboat"}
};
```

### 反馈上下文 (FMAFeedbackContext)
```cpp
struct FMAFeedbackContext
{
    FString TaskId;
    
    // 搜索结果
    TArray<FString> FoundObjects;         // 发现的对象名称
    TArray<FVector> FoundLocations;       // 发现的对象位置
    TMap<FString, FString> ObjectAttributes;  // 对象属性
    
    // 导航信息
    FString NearbyLandmarkLabel;          // 附近地标
    
    // 跟随信息
    FString FollowTargetRobotName;
    bool bFollowTargetFound;
};
```

### 技能执行反馈 (FMASkillExecutionFeedback)
```cpp
struct FMASkillExecutionFeedback
{
    FString AgentId;                      // 执行机器人 ID
    FString SkillName;                    // 技能名称
    bool bSuccess;                        // 是否成功
    FString Message;                      // 执行消息
    TMap<FString, FString> Data;          // 额外数据
};
```
