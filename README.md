# MultiAgent-Unreal

基于 Unreal Engine 5 的多智能体仿真框架，用于机器人、无人机、角色等异构智能体的协同仿真。

## 快速开始

### 环境要求
- Unreal Engine 5.3+
- Visual Studio 2022 / Rider

### 编译运行
1. 打开 `unreal_project/MultiAgent.uproject`
2. 编译项目
3. 运行 TopDownMap 关卡

---

## StateTree 配置指南

### 1. 创建 StateTree Asset

1. 在 Content Browser 中右键
2. 选择 **Miscellaneous** → **State Tree**
3. 命名为 `ST_RobotDog`（或其他名称）
4. 双击打开 StateTree 编辑器
5. 建议放在 `Content/AI/StateTree/` 目录下

### 2. 配置 Schema

在 **Asset Details** 面板中：

1. **Schema**: 选择 `StateTree AI Component`
2. **Context** 部分会自动出现：
   - **Actor**: 默认是 `Pawn`，可以保持默认或点击选择具体类（如 `MARobotDogCharacter`）
   - **AIController**: 默认是 `AIController`，可以保持默认

> 注意：保持 `Pawn` 默认值也可以工作，因为 Task/Condition 代码中会自己 Cast 到正确类型。

### 3. 创建状态结构

在 StateTree 编辑器主面板中：

1. 右键空白处 → **Add State** → 命名为 `Root`
2. 右键 `Root` → **Add Child State** → 创建子状态：
   - `Idle` - 空闲状态
   - `Patrol` - 巡逻状态
   - `Charge` - 充电状态

最终结构：
```
Root
├── Idle
├── Patrol
└── Charge
```

### 4. 为状态添加 Task

选中状态后，在右侧 **Details** 面板的 **Tasks** 部分：

**Idle 状态：**
- 不需要添加 Task

**Patrol 状态：**
1. 点击 **+** 添加 Task
2. 搜索 `MA Patrol`
3. 配置参数：
   - **Patrol Path**: 选择场景中的 `MAPatrolPath` Actor
   - **Wait Time At Waypoint**: 0.5
   - **Acceptance Radius**: 100

**Charge 状态：**
1. 添加 Task → `MA Activate Ability`
2. 配置：
   - **Ability Tag**: `Ability.Charge`
   - **Wait For Ability End**: true

### 5. 配置状态转换 (Transition)

**状态转移图：**
```
                         ┌──────────────┐
                         │     Idle     │ (默认)
                         └──────┬───────┘
                                │
         ┌──────────────────────┼──────────────────────┐
         │ Command.Patrol       │ Command.Charge       │
         ▼                      ▼                      │
  ┌──────────────┐       ┌──────────────┐             │
  │    Patrol    │       │    Charge    │             │
  │              │       │              │             │
  │ 电量<20%自动 │──────▶│              │─────────────┘
  │ 切换到Charge │       │ 电量=100%    │  Command.Idle
  └──────────────┘       │ 返回之前状态 │
                         └──────────────┘
```

**Transition 配置清单：**

| 从状态 | 到状态 | 条件 | Trigger |
|--------|--------|------|---------|
| **Idle** | Patrol | `MA Has Command` (Command.Patrol) | On Tick |
| **Idle** | Charge | `MA Has Command` (Command.Charge) | On Tick |
| **Patrol** | Charge | `MA Low Energy` (20%) | On Tick |
| **Patrol** | Charge | `MA Has Command` (Command.Charge) | On Tick |
| **Patrol** | Idle | `MA Has Command` (Command.Idle) | On Tick |
| **Charge** | Patrol | `MA Full Energy` + `MA Has Command` (Command.Patrol) | On Tick |
| **Charge** | Idle | `MA Full Energy` + `MA Has Command` (Command.Idle) | On Tick |

**配置步骤：**

1. 选中状态
2. 在 **Transitions** 部分点击 **+**
3. 设置 **Target State**
4. 设置 **Trigger** 为 `On Tick`
5. 在 **Conditions** 中添加条件
6. 如果需要多个条件同时满足，添加多个 Condition
7. 如果需要取反条件，勾选 **Invert**

### 6. 设置默认入口状态

1. 选中 `Root` 状态
2. 设置默认子状态为 `Idle`（机器人启动时先进入空闲状态）

### 7. 保存

按 **Ctrl+S** 保存 StateTree Asset


### 8. 将 StateTree 分配给 Character

**方式 A：创建蓝图**

1. 创建 `BP_RobotDog` 蓝图（继承自 `MARobotDogCharacter`）
2. 在 Components 中找到 `StateTreeComponent`
3. 在 Details 面板设置 **State Tree** 为 `ST_RobotDog`
4. 在场景中使用 `BP_RobotDog` 而不是 C++ 类

**方式 B：C++ 代码加载**

```cpp
void AMARobotDogCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    UStateTree* ST = LoadObject<UStateTree>(nullptr, 
        TEXT("/Game/AI/StateTree/ST_RobotDog.ST_RobotDog"));
    if (ST && StateTreeComponent)
    {
        StateTreeComponent->SetStateTree(ST);
    }
}
```

### 9. 外部发送命令

```cpp
// 让机器人开始巡逻
Robot->GetAbilitySystemComponent()->SendCommand(
    FMAGameplayTags::Get().Command_Patrol);

// 让机器人去充电
Robot->GetAbilitySystemComponent()->SendCommand(
    FMAGameplayTags::Get().Command_Charge);

// 让机器人停止
Robot->GetAbilitySystemComponent()->SendCommand(
    FMAGameplayTags::Get().Command_Idle);
```

### 10. 确保 Character 有 AIController

StateTree AI Component 需要 AIController 才能工作：

```cpp
AMARobotDogCharacter::AMARobotDogCharacter()
{
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();
}
```

---

## 键盘快捷键

| 按键 | 功能 |
|------|------|
| G | 发送 Command.Patrol（开始巡逻） |
| H | 发送 Command.Charge（去充电） |
| J | 发送 Command.Idle（停止） |
| K | 发送 Command.Coverage（区域覆盖扫描） |
| F | 发送 Command.Follow（跟随目标） |
| 左键 | 移动所有 Human |
| 右键 | 移动所有 RobotDog |
| P | 拾取物品 |
| O | 放下物品 |
| I | 生成可拾取方块 |
| T | 生成机器狗 |
| Tab | 切换摄像头视角 |
| 0 | 返回上帝视角 |

---

## StateTree Task 参数

### MA Charge

自动查找充电站，导航过去并充电。

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `ChargingStationTag` | "ChargingStation" | 充电站 Actor Tag |
| `AcceptanceRadius` | 150 | 到达判定距离 |

### MA Patrol

循环巡逻路径点。

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `PatrolPathTag` | "PatrolPath" | 巡逻路径 Actor Tag |
| `WaitTimeAtWaypoint` | 0.5 | 每个路径点等待时间 |
| `AcceptanceRadius` | 100 | 到达判定距离 |

### MA Navigate To Location

导航到指定位置。

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `TargetLocation` | (0,0,0) | 目标位置 |
| `AcceptanceRadius` | 100 | 到达判定距离 |

### MA Coverage

区域覆盖扫描（割草机模式）。

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `CoverageAreaTag` | "CoverageArea" | 覆盖区域 Actor Tag |
| `AcceptanceRadius` | 100 | 到达判定距离 |
| `WaitTimeAtPoint` | 0 | 每个点停留时间 |
| `bLoopOnComplete` | false | 完成后是否循环 |

路径生成特性：
- 使用机器人的 `ScanRadius` 属性计算路径间距
- 自动投影到 NavMesh，确保所有点可达
- 投影失败时尝试4个方向偏移，逐步扩大搜索半径

### MA Follow

跟随目标 Character。

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `TargetActorTag` | "Human" | 目标 Actor Tag |
| `FollowDistance` | 200 | 跟随距离 |

### MA Activate Ability

激活 GAS Ability。

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `AbilityTag` | - | 要激活的 Ability Tag |
| `WaitForAbilityEnd` | true | 是否等待 Ability 结束 |

## 可用的自定义 Condition

| Condition 名称 | 功能 | 参数 |
|----------------|------|------|
| `MA Low Energy` | 检查电量 < 阈值 | EnergyThreshold (默认 20%) |
| `MA Full Energy` | 检查电量 >= 阈值 | FullThreshold (默认 100%) |
| `MA Has Patrol Path` | 检查是否有可用巡逻路径 | PatrolPath (可选) |
| `MA Has Command` | 检查是否收到指定命令 | CommandTag |

## Command Tags

| Tag | 用途 |
|-----|------|
| `Command.Patrol` | 命令开始巡逻 |
| `Command.Charge` | 命令去充电 |
| `Command.Idle` | 命令停止/空闲 |
| `Command.Search` | 命令开始搜索 |
| `Command.Coverage` | 命令区域覆盖扫描 |
| `Command.Follow` | 命令跟随目标 |

---

## UE 资源路径映射

| 磁盘路径 | UE 内部路径 |
|---------|------------|
| `Content/` | `/Game/` |
| `Content/StateTree/ST_RobotDog.uasset` | `/Game/StateTree/ST_RobotDog` |

**在代码中使用：**
```cpp
LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_RobotDog.ST_RobotDog"));
```

---

## 常见问题

**Q: 找不到自定义 Task/Condition？**
A: 确保项目已编译，重启编辑器。

**Q: StateTree 不运行？**
A: 检查：
1. StateTreeComponent 是否设置了 StateTree Asset
2. Character 是否有 AIController
3. Schema 是否配置正确

**Q: Transition 不触发？**
A: 检查 Trigger 是否设置为 `On Tick`，以及 Condition 是否正确配置。

**Q: StateTree 不允许直接引用关卡 Actor？**
A: StateTree 是 Asset，不能直接引用关卡中的 Actor 实例。使用 Actor Tag 在运行时查找：
```cpp
TArray<AActor*> FoundActors;
UGameplayStatics::GetAllActorsWithTag(World, "PatrolPath", FoundActors);
```

**Q: Condition 报错 "Malformed node, missing instance value"？**
A: UE5 StateTree 要求所有 Condition/Task 必须定义 `GetInstanceDataType()`：
```cpp
USTRUCT()
struct FMyConditionInstanceData
{
    GENERATED_BODY()
};

virtual const UStruct* GetInstanceDataType() const override 
{ 
    return FMyConditionInstanceData::StaticStruct(); 
}
```

---

## 文档

- [架构设计](doc/ARCHITECTURE.md) - 系统架构、类继承、模块详解
- [功能特性](doc/feature.md) - 充电站、巡逻路径等功能说明
- [Bug 记录](doc/bug.md) - 开发过程中遇到的问题和解决方案
