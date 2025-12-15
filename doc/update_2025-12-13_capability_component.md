# 重构记录: Capability Component 模式

> 日期: 2025-12-13
> 类型: 架构重构
> 状态: ✅ 已完成，编译通过

## 背景

原架构中，`MARobotDogCharacter` 和 `MADroneCharacter` 都直接实现了 4 个 Interface (`IMAChargeable`, `IMAPatrollable`, `IMAFollowable`, `IMACoverable`)，导致大量重复代码（约 50 行/类）。

为了：
1. 消除代码重复
2. 支持运行时动态添加/移除能力
3. 更接近 ROS 的 Parameter Server 设计理念（论文卖点）

决定将 Interface 的属性和实现迁移到独立的 Component 中。

## 设计理念

```
ROS Parameter Server          UE5 Capability Component
─────────────────────         ────────────────────────
/robot/energy: 100            UMAEnergyComponent::Energy = 100
/robot/scan_radius: 200       UMAPatrolComponent::ScanRadius = 200
rosparam get/set              Component->GetEnergy() / SetEnergy()
```

## 新增文件

### Capability Components

| 文件 | 实现 Interface | 主要属性 |
|------|---------------|---------|
| `MAEnergyComponent.h/cpp` | `IMAChargeable` | Energy, MaxEnergy, DrainRate, LowEnergyThreshold |
| `MAPatrolComponent.h/cpp` | `IMAPatrollable` | PatrolPath, ScanRadius |
| `MAFollowComponent.h/cpp` | `IMAFollowable` | FollowTarget |
| `MACoverageComponent.h/cpp` | `IMACoverable` | CoverageArea, ScanRadius |
| `MACapabilityComponents.h` | - | 统一头文件 |

路径: `Agent/Component/Capability/`

## 修改文件

### Character 类

**MARobotDogCharacter.h/cpp**
- 移除: `IMAChargeable`, `IMAPatrollable`, `IMAFollowable`, `IMACoverable` 继承
- 移除: Energy, MaxEnergy, ScanRadius, PatrolPath, FollowTarget, CoverageArea 等属性
- 移除: Interface 方法实现 (GetEnergy, DrainEnergy, RestoreEnergy 等)
- 新增: 4 个 Capability Component 指针
- 新增: `OnEnergyDepleted()` 回调绑定

**MADroneCharacter.h/cpp**
- 同上

### StateTree Tasks

**MASTTask_Patrol.cpp**
- 新增: `GetCapabilityInterface<T>()` 辅助函数
- 修改: `Cast<IMAPatrollable>(Owner)` → `GetCapabilityInterface<IMAPatrollable>(Owner)`
- 修改: `Cast<IMAChargeable>(Owner)` → `GetCapabilityInterface<IMAChargeable>(Owner)`

**MASTTask_Follow.cpp**
- 同上模式

**MASTTask_Coverage.cpp**
- 同上模式

**MASTTask_Charge.cpp**
- 同上模式

### StateTree Conditions

**MASTCondition_LowEnergy.cpp**
- 修改: 使用 `FindComponentByClass<UMAEnergyComponent>()` 获取能量

**MASTCondition_FullEnergy.cpp**
- 同上

### GAS Abilities

**GA_Charge.cpp**
- 修改: 使用 `UMAEnergyComponent` 获取/设置能量
- 移除: 对 `AMARobotDogCharacter` 的硬编码依赖

**GA_Follow.cpp**
- 修改: 使用 `UMAPatrolComponent` 获取 ScanRadius

### Core

**MACommandManager.cpp**
- 修改: `SetAgentCommandProperties()` 通过 `FindComponentByClass<>()` 设置参数
- 移除: 对具体 Character 类型的 Cast

## 辅助函数

在 StateTree Task 中添加了通用辅助函数：

```cpp
template<typename T>
T* GetCapabilityInterface(AActor* Actor)
{
    if (!Actor) return nullptr;
    
    // 先尝试从 Actor 本身获取 (兼容旧代码)
    if (T* Interface = Cast<T>(Actor))
    {
        return Interface;
    }
    
    // 再从 Component 获取
    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);
    for (UActorComponent* Comp : Components)
    {
        if (T* Interface = Cast<T>(Comp))
        {
            return Interface;
        }
    }
    
    return nullptr;
}
```

## 架构对比

### 重构前

```
AMARobotDogCharacter : AMACharacter, IMAChargeable, IMAPatrollable, IMAFollowable, IMACoverable
    float Energy = 100.f;           // 重复
    float MaxEnergy = 100.f;        // 重复
    float ScanRadius = 200.f;       // 重复
    void DrainEnergy() { ... }      // 重复实现
    void RestoreEnergy() { ... }    // 重复实现

AMADroneCharacter : AMACharacter, IMAChargeable, IMAPatrollable, IMAFollowable, IMACoverable
    float Energy = 100.f;           // 完全相同
    float MaxEnergy = 100.f;        // 完全相同
    float ScanRadius = 300.f;       // 完全相同
    void DrainEnergy() { ... }      // 完全相同
    void RestoreEnergy() { ... }    // 完全相同
```

### 重构后

```
UMAEnergyComponent : UActorComponent, IMAChargeable
    float Energy, MaxEnergy, DrainRate;
    void DrainEnergy(), RestoreEnergy();  // 只写一次

AMARobotDogCharacter : AMACharacter
    UMAEnergyComponent* EnergyComponent;      // 组合
    UMAPatrolComponent* PatrolComponent;      // 组合
    UMAFollowComponent* FollowComponent;      // 组合
    UMACoverageComponent* CoverageComponent;  // 组合

AMADroneCharacter : AMACharacter
    UMAEnergyComponent* EnergyComponent;      // 同样组合，零重复
    UMAPatrolComponent* PatrolComponent;
    UMAFollowComponent* FollowComponent;
    UMACoverageComponent* CoverageComponent;
```

## 优势

| 方面 | 重构前 | 重构后 |
|------|--------|--------|
| 代码重复 | ~50 行/类 | 0 |
| 新增 Agent 类型 | 复制所有 Interface 实现 | 只需添加 Component |
| 修改能力逻辑 | 改多个文件 | 只改一个 Component |
| 运行时动态能力 | 不支持 | 支持 |
| 论文描述 | "Interface-based" | "Plug-and-Play Capability Components" |

## 兼容性

- `GetCapabilityInterface<T>()` 函数同时支持 Actor 直接实现 Interface 和 Component 实现 Interface
- 现有的 StateTree Asset 无需修改
- JSON 配置无需修改

## 后续可选优化

1. 将 `GetCapabilityInterface<T>()` 移到公共头文件避免重复定义
2. 为 Human 类型也添加部分 Capability Component
3. 添加 `UMAAvoidanceComponent` 封装避障参数
