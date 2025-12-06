# Bug 记录

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

> **注意**: StateTree 配置指南已移至 [README.md](../README.md)


---

#### Bug 7: 切换命令时状态不切换（Command Tag 未完全清除）

**问题**: 机器人在 Follow 状态时，按 G 键发送 Patrol 命令，但机器人仍然继续 Follow，不切换到 Patrol。

**现象**:
```
[PlayerController] OnStartPatrol called (G key)
// 但没有看到 Patrol Task 启动的日志，机器人继续跟随
```

**原因**: `OnStartPatrol` 函数只移除了 `Command.Idle` 和 `Command.Charge`，没有移除 `Command.Follow`。StateTree 检测到 `Command.Follow` 仍然存在，Follow 状态的 Enter Condition 仍然满足，不会退出。

**错误代码**:
```cpp
// 只移除了部分命令
ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
ASC->AddLooseGameplayTag(PatrolCommand);
```

**解决**: 发送新命令时，必须移除所有其他命令 Tag：
```cpp
// 清除所有其他命令
ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Patrol")));
ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Follow")));
ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Coverage")));
// 然后添加新命令
ASC->AddLooseGameplayTag(NewCommand);
```

**教训**: 使用 Gameplay Tag 作为 StateTree 状态切换条件时，发送新命令必须清除所有其他命令 Tag，否则旧状态的 Enter Condition 可能仍然满足，导致状态不切换。


---

#### Bug 8: Mass AI 编译错误 - ProcessorGroupNames 不存在

**问题**: Mass AI Processor 编译时报错 `no member named 'ProcessorGroupNames' in namespace 'UE::Mass'`。

**现象**:
```
error: no member named 'ProcessorGroupNames' in namespace 'UE::Mass'; did you mean 'FProcessorGroupDesc'?
ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
```

**原因**: UE5 的 Mass Entity 框架中不存在 `UE::Mass::ProcessorGroupNames` 命名空间。`ExecutionOrder.ExecuteInGroup` 是一个 `FName` 类型，可以设置为任意名称。

**错误代码**:
```cpp
ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Movement);
```

**解决**: 使用 `FName` 直接指定组名：
```cpp
ExecutionOrder.ExecuteInGroup = FName(TEXT("Movement"));
ExecutionOrder.ExecuteBefore.Add(FName(TEXT("Movement")));
```

**影响文件**:
- `MAMovementProcessor.cpp`
- `MAEnergyDrainProcessor.cpp`
- `MAPatrolProcessor.cpp`
- `MAFollowProcessor.cpp`
- `MAChargeProcessor.cpp`

**教训**: Mass AI 的 Processor 执行顺序通过 `FName` 指定组名，不是使用预定义的命名空间常量。

---

#### Bug 9: Mass AI 编译错误 - FMassEntityTemplateID 类型不存在

**问题**: `MAMassSubsystem.h` 编译时报错 `unknown type name 'FMassEntityTemplateID'`。

**现象**:
```
error: unknown type name 'FMassEntityTemplateID'
FMassEntityTemplateID RobotDogTemplateID;
```

**原因**: UE5 Mass Entity 框架中不存在 `FMassEntityTemplateID` 类型。创建 Entity 使用的是 `FMassArchetypeHandle`。

**错误代码**:
```cpp
// MAMassSubsystem.h
FMassEntityTemplateID RobotDogTemplateID;

// MAMassSubsystem.cpp
RobotDogTemplateID = EntityManager.CreateArchetype(CompositionDescriptor);
FMassEntityHandle Entity = EntityManager.CreateEntity(RobotDogTemplateID);
```

**解决**: 使用 `FMassArchetypeHandle` 类型：
```cpp
// MAMassSubsystem.h
#include "MassArchetypeTypes.h"
FMassArchetypeHandle RobotDogArchetype;

// MAMassSubsystem.cpp
RobotDogArchetype = EntityManager.CreateArchetype(CompositionDescriptor);
FMassEntityHandle Entity = EntityManager.CreateEntity(RobotDogArchetype);
```

**教训**: Mass Entity 使用 Archetype 模式管理 Entity 模板，`CreateArchetype()` 返回 `FMassArchetypeHandle`。

---

#### Bug 10: Mass AI 编译错误 - FMassEntityHandle 不支持 Blueprint

**问题**: `MAMassSubsystem.h` 中使用 `FMassEntityHandle` 参数的函数标记为 `BlueprintCallable` 时报错。

**现象**:
```
error: Type 'FMassEntityHandle' is not supported by blueprint. Function: SpawnRobotDog Parameter ReturnValue
error: Type 'TArray<FMassEntityHandle>' is not supported by blueprint. Function: GetAllRobots Parameter ReturnValue
```

**原因**: `FMassEntityHandle` 是 Mass Entity 框架的内部类型，不支持 Blueprint 暴露。

**错误代码**:
```cpp
UFUNCTION(BlueprintCallable, Category = "Mass|Spawn")
FMassEntityHandle SpawnRobotDog(FVector Location);

UFUNCTION(BlueprintCallable, Category = "Mass|Query")
TArray<FMassEntityHandle> GetAllRobots() const;
```

**解决**: 移除这些函数的 `UFUNCTION(BlueprintCallable)` 标记，仅保留 C++ 接口：
```cpp
// C++ only - FMassEntityHandle 不支持 Blueprint
FMassEntityHandle SpawnRobotDog(FVector Location);
TArray<FMassEntityHandle> GetAllRobots() const;
```

**教训**: Mass Entity 的 Handle 类型不支持 Blueprint，需要在 C++ 层面使用。如需 Blueprint 支持，需要创建包装函数或使用其他方式（如 Actor 代理）。

---

#### Bug 11: Mass StateTree 编译错误 - 基类不存在

**问题**: Mass StateTree Condition 使用 `FMassStateTreeConditionBase` 基类时报错。

**现象**:
```
error: Unable to find parent struct type for 'FMAMassSTCondition_LowEnergy' named 'FMassStateTreeConditionBase'
```

**原因**: UE5 Mass AI 框架中只有 `FMassStateTreeTaskBase` 和 `FMassStateTreeEvaluatorBase`，没有 `FMassStateTreeConditionBase`。Mass StateTree 的 Condition 使用标准的 `FStateTreeConditionBase`。

**错误代码**:
```cpp
#include "MassStateTreeTypes.h"

USTRUCT(meta = (DisplayName = "MA Mass Low Energy"))
struct FMAMassSTCondition_LowEnergy : public FMassStateTreeConditionBase  // 错误！
```

**解决**: 使用标准的 `FStateTreeConditionBase`：
```cpp
#include "StateTreeConditionBase.h"

USTRUCT(meta = (DisplayName = "MA Mass Low Energy"))
struct FMAMassSTCondition_LowEnergy : public FStateTreeConditionBase
{
    // 在 TestCondition 中将 Context 转换为 FMassStateTreeExecutionContext
};
```

**教训**: Mass StateTree 的 Task 使用 `FMassStateTreeTaskBase`，但 Condition 使用标准的 `FStateTreeConditionBase`，在实现中通过 `static_cast` 转换为 `FMassStateTreeExecutionContext` 来访问 Mass Entity 数据。

---

#### Bug 12: Mass AI 插件未启用

**问题**: 编译时警告 Mass AI 相关模块依赖未在 `.uproject` 中声明。

**现象**:
```
Warning: MultiAgent.uproject does not list plugin 'MassGameplay' as a dependency, but module 'MultiAgent' depends on 'MassCommon'.
Warning: MultiAgent.uproject does not list plugin 'MassAI' as a dependency, but module 'MultiAgent' depends on 'MassAIBehavior'.
```

**原因**: `Build.cs` 中添加了 Mass AI 模块依赖，但 `.uproject` 文件中没有启用对应的插件。

**解决**: 在 `MultiAgent.uproject` 的 Plugins 数组中添加：
```json
{
    "Name": "MassEntity",
    "Enabled": true
},
{
    "Name": "MassGameplay",
    "Enabled": true
},
{
    "Name": "MassAI",
    "Enabled": true
},
{
    "Name": "StructUtils",
    "Enabled": true
}
```

**教训**: 使用 UE5 插件模块时，需要同时在 `Build.cs` 添加模块依赖和在 `.uproject` 中启用插件。


---

#### Bug 13: 编队跟随时机器人抖动 (已解决)

**问题**: 机器人在编队跟随 Leader 时，到达目标位置后会不断抖动。

**现象**: 机器人到达编队位置后，反复启动和停止移动，导致视觉上的抖动。

**原因**: 没有迟滞机制 (Hysteresis)，机器人到达目标后立即检测到"需要移动"，然后又立即到达，形成循环。

**解决**: 在 MAActorSubsystem 中实现迟滞机制：
- `StartMoveThreshold = 150.f` - 距离超过此值才开始移动
- `StopMoveThreshold = 80.f` - 距离小于此值才停止移动
- 两个阈值的差值形成"死区"，防止抖动

```cpp
// 迟滞机制
if (!bIsMoving && Distance > StartMoveThreshold)
{
    // 开始移动
    bIsMoving = true;
}
else if (bIsMoving && Distance < StopMoveThreshold)
{
    // 停止移动
    bIsMoving = false;
}
```

**教训**: 实时跟随系统需要迟滞机制防止抖动，使用不同的启动/停止阈值。

---

#### Bug 14: 相机拍照图片过暗 (已解决)

**问题**: MACameraSensor 拍照保存的图片非常暗，几乎全黑。

**现象**: 调用 `TakePhoto()` 保存的图片亮度极低，无法看清内容。

**原因**: SceneCaptureComponent2D 的默认配置不正确：
1. CaptureSource 设置错误
2. Gamma 校正未正确应用
3. 后处理设置不当

**解决**: 参考 UnrealCV 的 LitCamSensor 实现，应用正确的相机配置：
```cpp
// 使用 FinalColorLDR 作为捕获源
CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

// 启用后处理
CaptureComponent->bCaptureEveryFrame = false;
CaptureComponent->bCaptureOnMovement = false;

// 正确的 Gamma 设置
RenderTarget->TargetGamma = 2.2f;
```

**教训**: UE5 的 SceneCaptureComponent2D 需要正确配置 CaptureSource 和 Gamma 才能获得正确的图像亮度。参考成熟项目（如 UnrealCV、CARLA）的实现。

---

#### Bug 15: GA_TakePhoto Ability 开销过大 (已解决)

**问题**: 使用 GAS Ability 实现拍照功能增加了不必要的复杂性。

**现象**: 拍照是一个简单的即时操作，但通过 GAS 实现需要：
- 创建 Ability 类
- 注册到 ASC
- 通过 Tag 激活
- 处理 Ability 生命周期

**解决**: 采用 CARLA 风格，直接在 MACameraSensor 上实现 `TakePhoto()` 方法：
```cpp
// 直接调用，无需 GAS
AMACameraSensor* Camera = ...;
Camera->TakePhoto();  // 简单直接
```

**清理内容**:
- 删除 GA_TakePhoto.h/cpp
- 移除 MAAbilitySystemComponent 中的相关代码
- 更新 PlayerController 直接调用 Sensor

**教训**: 不是所有功能都需要通过 GAS 实现。简单的即时操作（如拍照）直接在 Actor 上实现更简洁。CARLA 风格：参数和简单操作放在实体上，复杂行为才用 Ability。
