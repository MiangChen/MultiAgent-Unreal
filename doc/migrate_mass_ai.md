# Mass AI 架构指南

基于 UE5 Mass AI 框架的多智能体仿真架构设计。

## 1. Mass AI 核心概念

### 1.1 ECS 架构

Mass AI 采用 Entity Component System (ECS) 架构：

```
┌─────────────────────────────────────────────────────────────┐
│   E = Entity    只是一个 ID（Handle），没有数据和行为         │
│   C = Fragment  纯数据，没有行为                             │
│   S = Processor 纯行为，批量处理数据                         │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 概念类比

| Mass AI | 类比 | 说明 |
|---------|------|------|
| **Entity** | 订单/产品 | 由 Fragment 组成的实体 |
| **Handle** | 订单号 | 唯一标识一个 Entity |
| **Fragment** | 食材/数据 | 存储实际数据 |
| **Trait** | 菜谱/模板 | 定义 Entity 需要哪些 Fragment |
| **Processor** | 厨师/流水线 | 批量处理特定 Fragment |

### 1.3 为什么用 Mass AI

| 规模 | 传统 Actor | Mass AI |
|------|-----------|---------|
| <100 | ✅ 流畅 | 过度设计 |
| 100-500 | ⚠️ 卡顿 | ✅ 流畅 |
| 1000+ | ❌ 不可用 | ✅ 流畅 |
| 10000+ | ❌ 不可用 | ✅ 可行 |

---

## 2. 架构设计

### 2.1 Fragment 设计

```cpp
// 基础 Fragment
USTRUCT()
struct FMATransformFragment : public FMassFragment
{
    GENERATED_BODY()
    FTransform Transform;
};

USTRUCT()
struct FMAEnergyFragment : public FMassFragment
{
    GENERATED_BODY()
    float Energy = 100.f;
    float MaxEnergy = 100.f;
    float DrainRate = 1.f;
};

USTRUCT()
struct FMAMovementFragment : public FMassFragment
{
    GENERATED_BODY()
    FVector Velocity;
    float MaxSpeed = 600.f;
};

// 行为 Fragment
USTRUCT()
struct FMAPatrolFragment : public FMassFragment
{
    GENERATED_BODY()
    TWeakObjectPtr<AMAPatrolPath> PatrolPath;
    int32 CurrentWaypointIndex = 0;
    float WaitTimer = 0.f;
};

USTRUCT()
struct FMAFollowFragment : public FMassFragment
{
    GENERATED_BODY()
    FMassEntityHandle TargetEntity;
    float FollowDistance = 200.f;
};

USTRUCT()
struct FMACoverageFragment : public FMassFragment
{
    GENERATED_BODY()
    TWeakObjectPtr<AMACoverageArea> CoverageArea;
    TArray<FVector> PathPoints;
    int32 CurrentPointIndex = 0;
};

USTRUCT()
struct FMAChargeFragment : public FMassFragment
{
    GENERATED_BODY()
    TWeakObjectPtr<AMAChargingStation> ChargingStation;
    float ChargeRate = 20.f;
    bool bIsCharging = false;
};

// 状态 Fragment
USTRUCT()
struct FMACommandFragment : public FMassFragment
{
    GENERATED_BODY()
    FGameplayTag CurrentCommand;  // Command.Patrol, Command.Follow, etc.
};
```

### 2.2 Trait 设计

```cpp
// 机器狗 Trait
UCLASS()
class UMARobotDogTrait : public UMassEntityTraitBase
{
    GENERATED_BODY()
    
public:
    UPROPERTY(EditAnywhere)
    float DefaultEnergy = 100.f;
    
    UPROPERTY(EditAnywhere)
    float DefaultScanRadius = 200.f;
    
    virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext) const override
    {
        // 基础 Fragment
        BuildContext.AddFragment<FMATransformFragment>();
        BuildContext.AddFragment<FMAEnergyFragment>();
        BuildContext.AddFragment<FMAMovementFragment>();
        BuildContext.AddFragment<FMACommandFragment>();
        
        // 可选行为 Fragment（根据需要添加）
        BuildContext.AddFragment<FMAPatrolFragment>();
        BuildContext.AddFragment<FMAFollowFragment>();
        BuildContext.AddFragment<FMACoverageFragment>();
        BuildContext.AddFragment<FMAChargeFragment>();
        
        // Mass StateTree
        BuildContext.AddFragment<FMassStateTreeFragment>();
    }
};

// 人类 Trait
UCLASS()
class UMAHumanTrait : public UMassEntityTraitBase
{
    GENERATED_BODY()
    
    virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext) const override
    {
        BuildContext.AddFragment<FMATransformFragment>();
        BuildContext.AddFragment<FMAMovementFragment>();
        BuildContext.AddFragment<FMassStateTreeFragment>();
    }
};
```

### 2.3 Processor 设计

```cpp
// 能量消耗 Processor
UCLASS()
class UMAEnergyDrainProcessor : public UMassProcessor
{
    GENERATED_BODY()
    
    FMassEntityQuery EnergyQuery;
    
    virtual void ConfigureQueries() override
    {
        EnergyQuery.AddRequirement<FMAEnergyFragment>(EMassFragmentAccess::ReadWrite);
        EnergyQuery.AddRequirement<FMAMovementFragment>(EMassFragmentAccess::ReadOnly);
    }
    
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override
    {
        EntityManager.ForEachEntityChunk(EnergyQuery, Context,
            [](FMassExecutionContext& Context)
            {
                auto Energies = Context.GetMutableFragmentView<FMAEnergyFragment>();
                auto Movements = Context.GetFragmentView<FMAMovementFragment>();
                
                for (int32 i = 0; i < Context.GetNumEntities(); ++i)
                {
                    // 移动时消耗能量
                    if (Movements[i].Velocity.SizeSquared() > 0.f)
                    {
                        Energies[i].Energy -= Energies[i].DrainRate * Context.GetDeltaTimeSeconds();
                        Energies[i].Energy = FMath::Max(0.f, Energies[i].Energy);
                    }
                }
            }
        );
    }
};

// 巡逻 Processor
UCLASS()
class UMAPatrolProcessor : public UMassProcessor
{
    GENERATED_BODY()
    
    FMassEntityQuery PatrolQuery;
    
    virtual void ConfigureQueries() override
    {
        PatrolQuery.AddRequirement<FMATransformFragment>(EMassFragmentAccess::ReadOnly);
        PatrolQuery.AddRequirement<FMAPatrolFragment>(EMassFragmentAccess::ReadWrite);
        PatrolQuery.AddRequirement<FMAMovementFragment>(EMassFragmentAccess::ReadWrite);
        PatrolQuery.AddRequirement<FMACommandFragment>(EMassFragmentAccess::ReadOnly);
    }
    
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override
    {
        EntityManager.ForEachEntityChunk(PatrolQuery, Context,
            [](FMassExecutionContext& Context)
            {
                auto Transforms = Context.GetFragmentView<FMATransformFragment>();
                auto Patrols = Context.GetMutableFragmentView<FMAPatrolFragment>();
                auto Movements = Context.GetMutableFragmentView<FMAMovementFragment>();
                auto Commands = Context.GetFragmentView<FMACommandFragment>();
                
                for (int32 i = 0; i < Context.GetNumEntities(); ++i)
                {
                    // 只处理巡逻命令
                    if (Commands[i].CurrentCommand != FGameplayTag::RequestGameplayTag("Command.Patrol"))
                        continue;
                    
                    if (!Patrols[i].PatrolPath.IsValid())
                        continue;
                    
                    // 获取当前目标点
                    FVector Target = Patrols[i].PatrolPath->GetWaypoint(Patrols[i].CurrentWaypointIndex);
                    FVector Current = Transforms[i].Transform.GetLocation();
                    
                    // 计算移动方向
                    FVector Direction = (Target - Current).GetSafeNormal();
                    Movements[i].Velocity = Direction * Movements[i].MaxSpeed;
                    
                    // 到达检测
                    if (FVector::Dist(Current, Target) < 100.f)
                    {
                        Patrols[i].CurrentWaypointIndex = 
                            (Patrols[i].CurrentWaypointIndex + 1) % Patrols[i].PatrolPath->GetNumWaypoints();
                    }
                }
            }
        );
    }
};

// 跟随 Processor
UCLASS()
class UMAFollowProcessor : public UMassProcessor
{
    GENERATED_BODY()
    
    FMassEntityQuery FollowQuery;
    
    virtual void ConfigureQueries() override
    {
        FollowQuery.AddRequirement<FMATransformFragment>(EMassFragmentAccess::ReadOnly);
        FollowQuery.AddRequirement<FMAFollowFragment>(EMassFragmentAccess::ReadOnly);
        FollowQuery.AddRequirement<FMAMovementFragment>(EMassFragmentAccess::ReadWrite);
        FollowQuery.AddRequirement<FMACommandFragment>(EMassFragmentAccess::ReadOnly);
    }
    
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override
    {
        EntityManager.ForEachEntityChunk(FollowQuery, Context,
            [&EntityManager](FMassExecutionContext& Context)
            {
                auto Transforms = Context.GetFragmentView<FMATransformFragment>();
                auto Follows = Context.GetFragmentView<FMAFollowFragment>();
                auto Movements = Context.GetMutableFragmentView<FMAMovementFragment>();
                auto Commands = Context.GetFragmentView<FMACommandFragment>();
                
                for (int32 i = 0; i < Context.GetNumEntities(); ++i)
                {
                    if (Commands[i].CurrentCommand != FGameplayTag::RequestGameplayTag("Command.Follow"))
                        continue;
                    
                    // 通过 Handle 获取目标位置
                    if (!EntityManager.IsEntityValid(Follows[i].TargetEntity))
                        continue;
                    
                    auto* TargetTransform = EntityManager.GetFragmentDataPtr<FMATransformFragment>(Follows[i].TargetEntity);
                    if (!TargetTransform)
                        continue;
                    
                    FVector TargetPos = TargetTransform->Transform.GetLocation();
                    FVector MyPos = Transforms[i].Transform.GetLocation();
                    float Distance = FVector::Dist(MyPos, TargetPos);
                    
                    // 保持跟随距离
                    if (Distance > Follows[i].FollowDistance)
                    {
                        FVector Direction = (TargetPos - MyPos).GetSafeNormal();
                        Movements[i].Velocity = Direction * Movements[i].MaxSpeed;
                    }
                    else
                    {
                        Movements[i].Velocity = FVector::ZeroVector;
                    }
                }
            }
        );
    }
};

// 充电 Processor
UCLASS()
class UMAChargeProcessor : public UMassProcessor
{
    GENERATED_BODY()
    
    FMassEntityQuery ChargeQuery;
    
    virtual void ConfigureQueries() override
    {
        ChargeQuery.AddRequirement<FMAEnergyFragment>(EMassFragmentAccess::ReadWrite);
        ChargeQuery.AddRequirement<FMAChargeFragment>(EMassFragmentAccess::ReadWrite);
        ChargeQuery.AddRequirement<FMACommandFragment>(EMassFragmentAccess::ReadOnly);
    }
    
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override
    {
        EntityManager.ForEachEntityChunk(ChargeQuery, Context,
            [](FMassExecutionContext& Context)
            {
                auto Energies = Context.GetMutableFragmentView<FMAEnergyFragment>();
                auto Charges = Context.GetMutableFragmentView<FMAChargeFragment>();
                auto Commands = Context.GetFragmentView<FMACommandFragment>();
                
                for (int32 i = 0; i < Context.GetNumEntities(); ++i)
                {
                    if (Commands[i].CurrentCommand != FGameplayTag::RequestGameplayTag("Command.Charge"))
                        continue;
                    
                    if (Charges[i].bIsCharging)
                    {
                        // 渐进式充电
                        Energies[i].Energy += Charges[i].ChargeRate * Context.GetDeltaTimeSeconds();
                        Energies[i].Energy = FMath::Min(Energies[i].Energy, Energies[i].MaxEnergy);
                    }
                }
            }
        );
    }
};
```

---

## 3. 目录结构

```
unreal_project/Source/MultiAgent/
├── Mass/                           # Mass AI 模块
│   ├── Fragment/                   # 数据片段
│   │   ├── MATransformFragment.h
│   │   ├── MAEnergyFragment.h
│   │   ├── MAMovementFragment.h
│   │   ├── MAPatrolFragment.h
│   │   ├── MAFollowFragment.h
│   │   ├── MACoverageFragment.h
│   │   ├── MAChargeFragment.h
│   │   └── MACommandFragment.h
│   ├── Trait/                      # Entity 模板
│   │   ├── MARobotDogTrait.h
│   │   ├── MAHumanTrait.h
│   │   └── MADroneTrait.h
│   ├── Processor/                  # 批量处理器
│   │   ├── MAEnergyDrainProcessor.h
│   │   ├── MAPatrolProcessor.h
│   │   ├── MAFollowProcessor.h
│   │   ├── MACoverageProcessor.h
│   │   ├── MAChargeProcessor.h
│   │   └── MAMovementProcessor.h
│   └── Spawner/                    # Entity 生成
│       └── MAMassSpawner.h
├── Actor/                          # 场景 Actor（保留）
│   ├── MAPatrolPath.h
│   ├── MACoverageArea.h
│   └── MAChargingStation.h
├── Input/                          # 输入系统（保留）
│   ├── MAInputActions.h
│   └── MAPlayerController.h
└── Core/                           # 核心框架
    ├── MAGameMode.h
    └── MAMassSubsystem.h           # Mass Entity 管理
```

---

## 4. 命令系统

### 4.1 发送命令

```cpp
// PlayerController 发送命令给所有 Entity
void AMAPlayerController::OnStartPatrol(const FInputActionValue& Value)
{
    UMAMassSubsystem* MassSubsystem = GetWorld()->GetSubsystem<UMAMassSubsystem>();
    
    // 设置所有 RobotDog Entity 的命令
    MassSubsystem->SetCommandForAllRobots(FGameplayTag::RequestGameplayTag("Command.Patrol"));
}
```

### 4.2 Mass Subsystem

```cpp
UCLASS()
class UMAMassSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
    
public:
    // 生成 Entity
    FMassEntityHandle SpawnRobotDog(FVector Location);
    FMassEntityHandle SpawnHuman(FVector Location);
    
    // 批量设置命令
    void SetCommandForAllRobots(FGameplayTag Command);
    void SetCommandForEntity(FMassEntityHandle Entity, FGameplayTag Command);
    
    // 设置目标
    void SetFollowTarget(FMassEntityHandle Follower, FMassEntityHandle Target);
    void SetPatrolPath(FMassEntityHandle Entity, AMAPatrolPath* Path);
    
private:
    FMassEntityManager* EntityManager;
    TArray<FMassEntityHandle> RobotEntities;
    TArray<FMassEntityHandle> HumanEntities;
};
```

---

## 5. Mass StateTree 集成

Mass AI 支持 StateTree，用于复杂决策：

```cpp
// 在 Trait 中添加 StateTree Fragment
BuildContext.AddFragment<FMassStateTreeFragment>();

// StateTree 可以读写 Fragment 数据
// Condition: 检查 EnergyFragment.Energy < 20
// Task: 设置 CommandFragment.CurrentCommand = Command.Charge
```

---

## 6. 开发路线

### Phase 1: 基础框架 (2 周)
- [ ] 创建 Mass/ 目录结构
- [ ] 实现基础 Fragment（Transform、Energy、Movement）
- [ ] 实现 RobotDogTrait
- [ ] 实现 MAMassSubsystem
- [ ] 测试 Entity 生成

### Phase 2: 核心 Processor (3 周)
- [ ] 实现 MovementProcessor
- [ ] 实现 EnergyDrainProcessor
- [ ] 实现 PatrolProcessor
- [ ] 实现 FollowProcessor
- [ ] 实现 ChargeProcessor

### Phase 3: 命令系统 (1 周)
- [ ] 实现 CommandFragment
- [ ] 修改 PlayerController 发送命令
- [ ] 测试命令切换

### Phase 4: StateTree 集成 (2 周)
- [ ] 集成 Mass StateTree
- [ ] 迁移 Condition 逻辑
- [ ] 测试自动状态切换

### Phase 5: 可视化 (1 周)
- [ ] 实现 LOD 系统（近处生成 Actor）
- [ ] 调试可视化工具

---

## 7. 参考资源

- [City Sample 项目](https://www.unrealengine.com/marketplace/en-US/product/city-sample)
- [Mass AI 官方文档](https://docs.unrealengine.com/5.0/en-US/mass-entity-in-unreal-engine/)
- [UE5 Mass Framework 源码](Engine/Plugins/Runtime/MassEntity/)
- [GDC 2022: Building Open Worlds in UE5](https://www.youtube.com/watch?v=EbXakIuZPFo)
