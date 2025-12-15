// MACapabilityComponents.h
// 能力组件统一头文件 - 方便一次性引入所有能力组件

#pragma once

#include "MAEnergyComponent.h"
#include "MAPatrolComponent.h"
#include "MAFollowComponent.h"
#include "MACoverageComponent.h"

/**
 * 能力组件系统说明
 * 
 * 设计理念:
 *   - 类似 ROS 的 Parameter Server，但参数存储在 Agent 本地
 *   - 每个能力封装为独立 Component，实现对应 Interface
 *   - Agent 通过组合 Component 获得能力，无需继承
 * 
 * 组件列表:
 *   - UMAEnergyComponent   : IMAChargeable  - 能量系统
 *   - UMAPatrolComponent   : IMAPatrollable - 巡逻能力
 *   - UMAFollowComponent   : IMAFollowable  - 跟随能力
 *   - UMACoverageComponent : IMACoverable   - 覆盖扫描能力
 * 
 * 使用方式:
 * 
 *   1. 在 Character 构造函数中创建组件:
 *      EnergyComp = CreateDefaultSubobject<UMAEnergyComponent>(TEXT("EnergyComponent"));
 *      PatrolComp = CreateDefaultSubobject<UMAPatrolComponent>(TEXT("PatrolComponent"));
 * 
 *   2. StateTree Task 通过 Interface 获取参数:
 *      // 方式1: 从 Actor 查找实现了 Interface 的 Component
 *      if (IMAChargeable* Chargeable = Actor->FindComponentByInterface<UMAChargeable>())
 *      {
 *          float Energy = Chargeable->GetEnergy();
 *      }
 * 
 *      // 方式2: 直接获取 Component
 *      if (UMAEnergyComponent* EnergyComp = Actor->FindComponentByClass<UMAEnergyComponent>())
 *      {
 *          float Energy = EnergyComp->GetEnergy();
 *      }
 * 
 *   3. CommandManager 设置参数:
 *      if (UMAPatrolComponent* PatrolComp = Agent->FindComponentByClass<UMAPatrolComponent>())
 *      {
 *          PatrolComp->SetPatrolPath(Path);
 *      }
 * 
 * 优势:
 *   - 零代码重复: 能力逻辑只写一次
 *   - 热插拔: 运行时可添加/移除能力
 *   - 松耦合: Task 只依赖 Interface，不依赖具体 Character 类型
 *   - 易扩展: 新增能力只需创建新 Component
 */
