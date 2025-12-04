# Bug 记录

## Bug #1: 物品拾取后不跟随角色移动

**日期**: 2025-12-03

**现象**: 
- 机器人拾取物品后，物体保持悬浮在原位置
- 角色移动时，物体不跟随
- 执行 Drop 后，物体才有重力掉落

**调试过程**:
1. 添加详细日志发现 `AttachToComponent()` 返回成功
2. 日志显示 `RootComponent: CollisionComponent`，而不是 `MeshComponent`
3. 发现问题：MeshComponent 有独立物理模拟，不跟随父组件移动

**根本原因**: 
UE 组件层级与物理模拟的关系问题

```
错误的层级：
Actor
└── CollisionComponent (Root, 无物理) ← AttachToComponent 移动的是这个
    └── MeshComponent (有物理) ← 物理引擎独立控制，不跟随移动

正确的层级：
Actor  
└── MeshComponent (Root, 有物理) ← 现在 Attach 移动的是可见的 Mesh
    └── CollisionComponent ← 跟着 Mesh 走
```

**核心原则**:
1. `AttachToComponent()` 只移动 Actor 的 RootComponent
2. 有物理模拟的组件会被物理引擎独立控制位置
3. **想要附着移动的组件必须是 Root，且关闭物理后才会跟随父组件**

**修复方案**:
修改 `MAPickupItem.cpp`，把 MeshComponent 设为 Root：

```cpp
// 修复前
RootComponent = CollisionComponent;
MeshComponent->SetupAttachment(RootComponent);

// 修复后
RootComponent = MeshComponent;
CollisionComponent->SetupAttachment(RootComponent);
```

**相关文件**:
- `Source/MultiAgent/Interaction/MAPickupItem.cpp`
- `Source/MultiAgent/GAS/Abilities/GA_Pickup.cpp`


---

## Bug #2: Human Agent 导航时不播放行走动画

**日期**: 2025-12-03

**现象**: 
- Human Agent 使用 `AIController->MoveToLocation()` 导航时，角色滑行移动
- 没有播放行走/跑步动画
- 动画蓝图 (ABP_Manny) 需要加速度输入才能驱动动画

**根本原因**: 
UE 的动画蓝图 (ABP_Manny) 依赖 `CharacterMovementComponent` 的加速度来混合行走/跑步动画。
`AIController->MoveToLocation()` 只设置目标位置，不会自动添加加速度输入。

**调试过程**:
1. 发现 `GetCharacterMovement()->Velocity` 有值，但动画不播放
2. 检查动画蓝图，发现它读取的是加速度方向，不是速度
3. 需要手动调用 `AddInputVector()` 来提供加速度输入

**修复方案**:
在导航期间每帧调用 `AddInputVector()` 提供加速度方向：

```cpp
// MAHumanAgent.cpp
void AMAHumanAgent::OnNavigationTick()
{
    FVector Velocity = GetCharacterMovement()->Velocity;
    if (Velocity.Size2D() > 3.0f)
    {
        FVector AccelDir = Velocity.GetSafeNormal2D();
        GetCharacterMovement()->AddInputVector(AccelDir);
    }
}
```

**设计决策**:
- 使用虚函数 `OnNavigationTick()` 让不同 Agent 类型可以有不同的动画驱动逻辑
- Human 需要加速度输入，Drone 不需要
- 基类提供空实现，子类按需重写

**相关文件**:
- `Source/MultiAgent/AgentManager/MAAgent.h/cpp` - 添加虚函数
- `Source/MultiAgent/AgentManager/MAHumanAgent.h/cpp` - 重写实现


---

## Bug #3: NavMesh 导航目标位置不在 NavMesh 上时报错

**日期**: 2025-12-04

**现象**: 
- 当导航目标位置不在 NavMesh 上时（如高台边缘、NavMesh 外的区域），`MoveToLocation()` 返回 `Failed`
- 控制台输出警告：`[Navigate] MoveToLocation failed for XXX`
- Character 停在原地不动

**根本原因**: 
UE 的 `AIController->MoveToLocation()` 默认要求目标位置必须在 NavMesh 上。如果目标位置不可达（不在 NavMesh 上），导航请求会直接失败。

**影响范围**:
- `GA_Navigate` - 直接导航到指定位置
- `GA_Follow` - 追踪目标时计算的跟随位置可能不在 NavMesh 上

**临时解决方案**:
在 `GA_Follow` 中已实现 fallback 机制：

```cpp
// GA_Follow.cpp - UpdateFollow()
EPathFollowingRequestResult::Type Result = AICtrl->MoveToLocation(
    FollowLocation,
    50.f,
    true, true,
    true,  // bProjectDestinationToNavigation - 自动投影到 NavMesh
    true
);

if ((int32)Result == 0)  // Failed
{
    // Fallback: 直接导航到目标位置
    Result = AICtrl->MoveToLocation(
        CurrentTargetLocation,
        FollowDistance,  // 用跟随距离作为接受半径
        true, true, true, true
    );
}
```

**待优化方案**:
1. 在 `GA_Navigate` 中也添加 `bProjectDestinationToNavigation = true` 参数
2. 或者在导航前先用 `UNavigationSystemV1::ProjectPointToNavigation()` 预处理目标位置
3. 添加更友好的错误提示和 fallback 行为

**相关文件**:
- `Source/MultiAgent/GAS/Abilities/GA_Navigate.cpp`
- `Source/MultiAgent/GAS/Abilities/GA_Follow.cpp`

**状态**: 🟡 部分解决（GA_Follow 有 fallback，GA_Navigate 待优化）
