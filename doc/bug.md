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
