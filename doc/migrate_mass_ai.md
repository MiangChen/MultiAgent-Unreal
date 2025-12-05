# Mass AI 迁移指南

本文档分析当前架构与 Mass AI 的对应关系，以及迁移策略。

## 1. 为什么需要 Mass AI

| 规模 | 当前架构 (Actor) | Mass AI (ECS) |
|------|------------------|---------------|
| <100 | ✅ 流畅 | 过度设计 |
| 100-500 | ⚠️ 开始卡顿 | ✅ 流畅 |
| 1000+ | ❌ 不可用 | ✅ 流畅 |
| 10000+ | ❌ 不可用 | ✅ 可行 |

**当前架构瓶颈：**
- 每个 Actor 都有完整的组件栈（Mesh、Collision、Movement、ASC、StateTree）
- 每帧都要 Tick 所有组件
- 内存占用大，Cache 不友好

**Mass AI 优势：**
- Entity 只是一个 ID + 数据片段
- 批量处理同类 Entity（SIMD 友好）
- 远处 Entity 不需要完整 Actor

---

## 2. 当前架构分析

### 2.1 模块清单

```
当前架构
├── Character/           # ACharacter 派生类
│   ├── MACharacter      # 基类
│   ├── MAHumanCharacter
│   └── MARobotDogCharacter
├── Actor/               # 场景 Actor
│   ├── MAPatrolPath
│   ├── MACoverageArea
│   └── MAChargingStation
├── GAS/                 # Gameplay Ability System
│   ├── MAAbilitySystemComponent
│   └── Ability/         # 11 个技能
├── StateTree/           # 状态决策
│   ├── Task/            # 6 个 Task
│   └── Condition/       # 4 个 Condition
├── Input/               # 输入系统
│   ├── MAInputActions
│   └── MAPlayerController
└── Core/                # 核心框架
    ├── MAActorSubsystem
    └── MAGameMode
```

### 2.2 复用性分析

| 模块 | 复用程度 | 说明 |
|------|----------|------|
| **Input/** | ✅ 100% | PlayerController 不变，继续发命令 |
| **Actor/** | ✅ 100% | PatrolPath、CoverageArea 等场景 Actor 不变 |
| **Core/MAGameMode** | ✅ 90% | 改为生成 Mass Entity 而非 Actor |
| **StateTree/Condition** | ✅ 80% | 逻辑可复用，需适配 Mass 数据结构 |
| **StateTree/Task** | ⚠️ 50% | 逻辑可复用，但需要重写为 Mass Processor |
| **GAS/Ability** | ⚠️ 30% | Mass Entity 不支持 GAS，需改为 Processor |
| **Character/** | ❌ 0% | 完全替换为 Mass Entity + Trait |

---

## 3. Mass AI 核心概念

### 3.1 概念对照表

| 当前架构 | Mass AI | 说明 |
|----------|---------|------|
| `AMACharacter` | `FMassEntityHandle` | 实体标识 |
| `UPROPERTY` 属性 | `FMassFragment` | 数据片段 |
| `UActorComponent` | `FMassTrait` | 定义 Entity 模板 |
| `Tick()` 函数 | `FMassProcessor` | 批量处理逻辑 |
| `UStateTreeComponent` | `FMassStateTreeProcessor` | Mass 版 StateTree |
| `UAbilitySystemComponent` | 自定义 Processor | Mass 不支持 GAS |

### 3.2 数据结构映射

**当前 MARobotDogCharacter 属性：**
```cpp
// 当前：存储在 Actor 上
class AMARobotDogCharacter
{
    float Energy;
    float ScanRadius;
    AMACharacter* FollowTarget;
    AMAPatrolPath* PatrolPath;
    AActor* CoverageArea;
};
```

**Mass AI Fragment 设计：**
```cpp
// Mass AI：存储在 Fragment 中
USTRUCT()
struct FMARobotFragment : public FMassFragment
{
    GENERATED_BODY()
    
    float Energy = 100.f;
    float ScanRadius = 200.f;
};

USTRUCT()
struct FMAFollowFragment : public FMassFragment
{
    GENERATED_BODY()
    
    FMassEntityHandle TargetEntity;
    float FollowDistance;
};

USTRUCT()
struct FMAPatrolFragment : public FMassFragment
{
    GENERATED_BODY()
    
    TWeakObjectPtr<AMAPatrolPath> PatrolPath;
    int32 CurrentWaypointIndex;
};
```

---

## 4. 迁移策略

### 4.1 分阶段迁移

```
Phase 1: 混合架构 (推荐先做)
├── 保留当前 Actor 架构用于"重要"单位
├── 新增 Mass Entity 用于"群众"单位
└── 两者共存，逐步迁移

Phase 2: 完全 Mass AI
├── 所有单位都是 Mass Entity
├── 近处 Entity 生成临时 Actor（用于动画、碰撞）
└── 远处 Entity 纯数据
```

### 4.2 Phase 1 实现方案

**保留 Actor 的场景：**
- 玩家直接交互的单位
- 需要复杂动画的单位
- 需要物理碰撞的单位
- 数量 < 50

**使用 Mass Entity 的场景：**
- 背景 AI（行人、车辆）
- 大规模集群（蜂群、鱼群）
- 简单行为的单位
- 数量 > 100

### 4.3 代码迁移示例

**当前 StateTree Task (MASTTask_Patrol)：**
```cpp
EStateTreeRunStatus FMASTTask_Patrol::Tick(float DeltaTime)
{
    // 获取 Actor
    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Context.GetActor());
    
    // 获取巡逻路径
    AMAPatrolPath* Path = Robot->GetPatrolPath();
    
    // 导航到下一个点
    Robot->TryNavigateTo(Path->GetWaypoint(CurrentIndex));
}
```

**Mass AI Processor (MMAPatrolProcessor)：**
```cpp
void UMAPatrolProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    // 批量处理所有有 PatrolFragment 的 Entity
    EntityManager.ForEachEntityChunk(
        PatrolQuery,
        Context,
        [](FMassExecutionContext& Context)
        {
            auto Transforms = Context.GetMutableFragmentView<FTransformFragment>();
            auto Patrols = Context.GetMutableFragmentView<FMAPatrolFragment>();
            auto Movements = Context.GetMutableFragmentView<FMassMoveTargetFragment>();
            
            for (int32 i = 0; i < Context.GetNumEntities(); ++i)
            {
                // 获取当前路径点
                FVector Target = Patrols[i].PatrolPath->GetWaypoint(Patrols[i].CurrentIndex);
                
                // 设置移动目标
                Movements[i].Center = Target;
            }
        }
    );
}
```

---

## 5. 模块迁移详情

### 5.1 StateTree → Mass StateTree

**好消息：** UE5.1+ 支持 Mass StateTree，概念基本相同。

| 当前 | Mass StateTree |
|------|----------------|
| `FStateTreeTaskBase` | `FMassStateTreeTaskBase` |
| `FStateTreeConditionBase` | `FMassStateTreeConditionBase` |
| `UStateTreeComponent` | `FMassStateTreeTrait` |

**迁移工作量：** 中等
- Condition 逻辑基本可复用
- Task 需要改为操作 Fragment 而非 Actor

### 5.2 GAS → Mass Processor

**坏消息：** Mass Entity 不支持 GAS，需要自己实现。

| 当前 GAS Ability | Mass 替代方案 |
|------------------|---------------|
| `GA_Navigate` | `FMassMoveTargetFragment` + 内置导航 |
| `GA_Follow` | `FMAFollowProcessor` |
| `GA_Patrol` | `FMAPatrolProcessor` |
| `GA_Charge` | `FMAChargeProcessor` |
| `GA_Search` | `FMASearchProcessor` |
| `GA_Coverage` | `FMACoverageProcessor` |

**迁移工作量：** 大
- 需要重写所有 Ability 为 Processor
- 失去 GAS 的 Tag、冷却、消耗等功能
- 需要自己实现状态管理

### 5.3 Character → Entity + Trait

**当前 Character 类：**
```cpp
AMARobotDogCharacter
├── USkeletalMeshComponent
├── UCharacterMovementComponent
├── UMAAbilitySystemComponent
├── UMAStateTreeComponent
└── 自定义属性 (Energy, ScanRadius, etc.)
```

**Mass Entity 设计：**
```cpp
// Trait 定义 Entity 模板
UCLASS()
class UMARobotDogTrait : public UMassEntityTraitBase
{
    // 包含的 Fragment
    FMARobotFragment RobotData;
    FMassMovementFragment Movement;
    FMassStateTreeFragment StateTree;
    
    // 可选：近处生成 Actor
    FMassActorFragment ActorInfo;
};
```

**迁移工作量：** 大
- 需要重新设计数据结构
- 需要实现 LOD 系统（远处纯数据，近处生成 Actor）

---

## 6. 推荐迁移路线

### Step 1: 学习 Mass AI 基础 (1-2 周)
- [ ] 下载 City Sample 项目学习
- [ ] 阅读 Mass AI 官方文档
- [ ] 创建简单的 Mass Entity 测试

### Step 2: 实现混合架构 (2-3 周)
- [ ] 保留当前 Actor 架构
- [ ] 新增 Mass Entity 子系统
- [ ] 实现简单的 Mass Entity（如背景行人）
- [ ] 验证两者共存

### Step 3: 迁移 StateTree (2-3 周)
- [ ] 将 Condition 迁移到 Mass StateTree
- [ ] 将 Task 迁移到 Mass StateTree
- [ ] 测试 Mass StateTree 决策

### Step 4: 迁移 Ability (3-4 周)
- [ ] 实现 Mass 版导航 Processor
- [ ] 实现 Mass 版巡逻 Processor
- [ ] 实现 Mass 版跟随 Processor
- [ ] 实现其他 Processor

### Step 5: 实现 LOD 系统 (2-3 周)
- [ ] 近处 Entity 生成临时 Actor
- [ ] 远处 Entity 纯数据
- [ ] 平滑过渡

---

## 7. 风险与注意事项

### 7.1 风险

| 风险 | 影响 | 缓解措施 |
|------|------|---------|
| Mass AI 文档少 | 学习曲线陡峭 | 参考 City Sample 源码 |
| 失去 GAS 功能 | 需要自己实现 | 只迁移简单行为的单位 |
| 调试困难 | 开发效率下降 | 保留 Actor 架构用于调试 |
| UE 版本依赖 | 需要 UE5.1+ | 确认项目 UE 版本 |

### 7.2 不建议迁移的场景

- 单位数量 < 500
- 需要复杂 GAS 技能系统
- 需要精确物理碰撞
- 开发时间紧张

### 7.3 建议迁移的场景

- 单位数量 > 1000
- 行为相对简单（巡逻、跟随、群集）
- 性能是关键指标
- 有足够开发时间

---

## 8. 参考资源

- [City Sample 项目](https://www.unrealengine.com/marketplace/en-US/product/city-sample) - Epic 官方 Mass AI 示例
- [Mass AI 官方文档](https://docs.unrealengine.com/5.0/en-US/mass-entity-in-unreal-engine/)
- [UE5 Mass Framework 源码](Engine/Plugins/Runtime/MassEntity/)
- [GDC 2022: Building Open Worlds in UE5](https://www.youtube.com/watch?v=EbXakIuZPFo)

---

## 9. 总结

| 方面 | 评估 |
|------|------|
| **复用率** | ~50% (Input、Actor、部分 StateTree 逻辑) |
| **重写量** | ~50% (Character、GAS、部分 StateTree) |
| **预计工期** | 10-15 周 (全量迁移) |
| **推荐策略** | 混合架构，逐步迁移 |

**核心结论：**
1. 当前架构的 **设计理念** 可以复用（CARLA 风格参数管理、StateTree 决策）
2. 当前架构的 **代码实现** 需要重写（Actor → Entity，GAS → Processor）
3. 建议采用 **混合架构**，保留 Actor 用于重要单位，Mass Entity 用于群众单位
