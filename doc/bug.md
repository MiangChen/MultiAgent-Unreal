# Bug 记录与 StateTree 配置指南

---

## 第一部分：Bug 记录

### 2024-12-04: ChargingStation 功能调试

#### Bug 1: Static Mesh vs Actor 混淆

**问题**: 将 Static Mesh (`SM_StationRecharge`) 直接拖入场景，而不是 `AMAChargingStation` Actor。

**现象**: 场景中有充电站模型，但没有任何充电逻辑触发，没有 Overlap 事件。

**原因**: Static Mesh 只是模型资产，没有 C++ 类的逻辑（Overlap 检测、充电功能等）。

**解决**: 
- C++ Actor 类需要通过 **Place Actors** 面板搜索并拖入场景
- 或者创建 **Blueprint 子类**（如 `BP_ChargingStation`）后拖入场景
- 不能直接拖 Static Mesh 到场景

**教训**: UE 中 Static Mesh 和 Actor 是不同概念：
- Static Mesh: 纯模型资产，无逻辑
- Actor: 可包含组件、逻辑、事件的游戏对象

---

#### Bug 2: Gameplay Tags 未注册

**问题**: 新增的 Gameplay Tags（如 `Ability.Charge`, `Status.Charging`）报错 "was not found"。

**现象**: 
```
Error: Requested Gameplay Tag Ability.Charge was not found, tags must be loaded from config or registered as a native tag
```

**原因**: 新增的 Tags 没有在配置文件中注册。

**解决**: 创建 `Config/DefaultGameplayTags.ini` 文件，添加所有需要的 Tags：
```ini
+GameplayTagList=(Tag="Ability.Charge",DevComment="")
+GameplayTagList=(Tag="Status.Charging",DevComment="")
+GameplayTagList=(Tag="Event.Charge.Complete",DevComment="")
```

**教训**: 新增 Gameplay Tags 时，必须同时在配置文件中注册，否则运行时会报错。

---

#### Bug 3: Patrol Task 路径点循环卡住

**问题**: RobotDog 到达第一个路径点后，无法前进到下一个路径点，陷入无限循环。

**现象**: 
```
[Navigate] RobotDog_4 already at goal
[STTask_Patrol] RobotDog_4 moving to WP[1/4] (1240, 1900)
[Navigate] RobotDog_4 already at goal
[STTask_Patrol] RobotDog_4 moving to WP[1/4] (1240, 1900)
... (无限重复)
```

**日志分析**:
- ✅ StateTree 正常启动 (`RunStatus: Running`)
- ✅ 找到了 PatrolPath (4个路径点)
- ✅ 开始移动到第一个路径点 WP[1/4] (1240, 1900)
- ❌ 到达后一直重复移动到同一个点，没有进入下一个路径点

**根本原因**: Patrol Task 的 Tick 逻辑中，状态机转换有问题：

当 `Character->bIsMoving` 变成 `false` 时（到达目标），代码应该：
1. 设置 `Data.bIsMoving = false`
2. 进入等待状态 (`Data.bIsWaiting = true`)
3. 等待结束后，移动到下一个路径点 (`Data.CurrentWaypointIndex++`)

但实际上代码一直重复调用移动到同一个路径点，没有正确处理"到达路径点"的状态转换。

**解决**: 修复 `MASTTask_Patrol.cpp` 中的 Tick 函数，确保：
1. 检测到 `!Character->bIsMoving && Data.bIsMoving == true` 时，正确设置 `Data.bIsMoving = false`
2. 进入等待状态或直接递增 `CurrentWaypointIndex`
3. 避免在同一帧内重复发起移动请求

**教训**: StateTree Task 的状态机逻辑需要仔细处理状态转换边界条件，特别是"完成"状态的检测和后续处理。

---

#### Bug 4: StateTree 无法正常加载/运行

**问题**: StateTree 配置完成后，运行时没有任何反应，状态机不执行。

**可能原因及解决方案**:

1. **StateTree Asset 未分配**
   - 检查 `StateTreeComponent` 的 `StateTree` 属性是否设置
   - 如果用 C++ 加载，确认路径正确

2. **缺少 AIController**
   - StateTree AI Component Schema 需要 AIController
   - 确保 Character 设置了 `AIControllerClass` 和 `AutoPossessAI`
   ```cpp
   AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
   AIControllerClass = AAIController::StaticClass();
   ```

3. **Schema 配置错误**
   - StateTree Asset 的 Schema 必须选择 `StateTree AI Component Schema`
   - 不是 `StateTree Component Schema`（这个不需要 AIController 但功能不同）

4. **StateTree 结构不完整**
   - 必须有 Root 状态和至少一个子状态
   - 子状态需要有 Task 或 Transition

5. **编译问题**
   - 自定义 Task/Condition 代码有错误会导致整个 StateTree 无法运行
   - 检查 Output Log 是否有编译错误

**教训**: StateTree 不工作时，按以下顺序排查：Asset 分配 → AIController → Schema → 结构完整性 → 编译错误。

---

#### Bug 5: PatrolPath 找不到 (Actor Tag 查找失败)

**问题**: Patrol Task 运行时报错 "No PatrolPath found with tag 'PatrolPath'"。

**现象**:
```
[STTask_Patrol] ========== EnterState called ==========
[STTask_Patrol] No PatrolPath found with tag 'PatrolPath'
```

**原因**: 场景中的 `MAPatrolPath` Actor 没有设置正确的 Actor Tag。

**解决**:
1. 在场景中选中 `MAPatrolPath` Actor
2. 在 Details 面板找到 **Actor** → **Tags** 部分
3. 点击 **+** 添加 Tag: `PatrolPath`
4. 确保 Tag 名称与代码中查找的名称完全一致（区分大小写）

**代码中的查找逻辑**:
```cpp
TArray<AActor*> FoundActors;
UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("PatrolPath"), FoundActors);
```

**教训**: 使用 Actor Tag 进行运行时查找时，必须确保：
- 场景中的 Actor 已添加对应 Tag
- Tag 名称完全匹配（大小写敏感）
- 如果有多个同类 Actor，考虑使用不同 Tag 区分

---

#### Bug 6: 软引用路径问题

**问题**: 硬编码的资产路径在资产移动/重命名后会失效。

**解决**: 使用 `TSoftObjectPtr` 软引用，让 UE 自动追踪资产路径变化：
```cpp
UPROPERTY(EditAnywhere)
TSoftObjectPtr<UStaticMesh> StationMeshAsset;
```

**教训**: 避免在 C++ 中硬编码资产路径，使用软引用或在编辑器中配置。

---

## 第二部分：StateTree 配置指南

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

### 5. 配置状态转换 (Transition) - 命令驱动模式

**状态转移图（命令驱动）：**
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

## 可用的自定义 Task

| Task 名称 | 功能 | 参数 |
|-----------|------|------|
| `MA Patrol` | 循环巡逻路径点 | PatrolPath, WaitTime, AcceptanceRadius |
| `MA Navigate To Location` | 导航到指定位置 | TargetLocation, AcceptanceRadius |
| `MA Activate Ability` | 激活 GAS Ability | AbilityTag, WaitForAbilityEnd |

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

---

## UE 资源路径映射

Unreal Engine 中磁盘路径和内部路径的对应关系：

| 磁盘路径 | UE 内部路径 |
|---------|------------|
| `Content/` | `/Game/` |
| `Content/StateTree/ST_RobotDog.uasset` | `/Game/StateTree/ST_RobotDog` |

**说明：**
- `/Game/` 是 UE 对项目 `Content` 文件夹的虚拟挂载点
- `.uasset` 扩展名在代码中不需要写

**完整引用格式：**
```
/Script/StateTreeModule.StateTree'/Game/StateTree/ST_RobotDog.ST_RobotDog'
```
- `/Script/StateTreeModule.StateTree` = 资源类型
- `/Game/StateTree/ST_RobotDog` = 包路径
- `.ST_RobotDog` = 资源名称（通常和包名相同）

**在代码中使用：**
```cpp
// 这两种写法都可以
LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_RobotDog.ST_RobotDog"));
LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_RobotDog"));
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

**Q: Context Actor Class 找不到我的类？**
A: 可以保持默认的 `Pawn`，代码中会自动 Cast。

**Q: Transition 不触发？**
A: 检查 Trigger 是否设置为 `On Tick`，以及 Condition 是否正确配置。

**Q: StateTree 不允许直接引用关卡 Actor？**
A: StateTree 是 Asset，不能直接引用关卡中的 Actor 实例。解决方案：
1. 使用 `TSoftObjectPtr` 软引用
2. 使用 Actor Tag 在运行时查找（推荐）
```cpp
// 通过 Tag 查找
TArray<AActor*> FoundActors;
UGameplayStatics::GetAllActorsWithTag(World, "PatrolPath", FoundActors);
```

**Q: Condition 报错 "Malformed node, missing instance value"？**
A: UE5 StateTree 要求所有 Condition/Task 必须定义 `GetInstanceDataType()`：
```cpp
// 即使不需要实例数据也要定义空的 struct
USTRUCT()
struct FMyConditionInstanceData
{
    GENERATED_BODY()
};

// 在 Condition 中返回
virtual const UStruct* GetInstanceDataType() const override 
{ 
    return FMyConditionInstanceData::StaticStruct(); 
}
```
