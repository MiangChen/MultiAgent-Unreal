// MAGameplayTags.h
// 项目统一的 Gameplay Tags 定义
// State Tree 和 GAS 通过这些 Tags 进行通信

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * 项目 Gameplay Tags 单例管理器
 * 
 * Tag 命名规范:
 * - State.XXX: 状态标签 (State Tree 使用)
 * - Ability.XXX: 技能标签 (GAS 使用)
 * - Event.XXX: 事件标签 (ST <-> GAS 通信)
 */
struct MULTIAGENT_API FMAGameplayTags
{
public:
    static const FMAGameplayTags& Get() { return GameplayTags; }
    static void InitializeNativeTags();

    // ========== State Tags (State Tree 状态) ==========
    FGameplayTag State_Exploration;      // 探索模式
    FGameplayTag State_PhotoMode;        // 拍照模式
    FGameplayTag State_Interaction;      // 交互模式
    FGameplayTag State_Interaction_Pickup;    // 拾取中
    FGameplayTag State_Interaction_Dialogue;  // 对话中

    // ========== Ability Tags (GAS 技能) ==========
    FGameplayTag Ability_Pickup;         // 拾取技能
    FGameplayTag Ability_Drop;           // 放下技能
    FGameplayTag Ability_TakePhoto;      // 拍照技能
    FGameplayTag Ability_Navigate;       // 导航技能
    FGameplayTag Ability_Interact;       // 交互技能
    FGameplayTag Ability_Patrol;         // 巡逻技能
    FGameplayTag Ability_Search;         // 搜索技能
    FGameplayTag Ability_Observe;        // 观察技能
    FGameplayTag Ability_Report;         // 报告技能
    FGameplayTag Ability_Charge;         // 充电技能
    FGameplayTag Ability_Formation;      // 编队技能
    FGameplayTag Ability_Avoid;          // 避障技能

    // ========== Event Tags (ST <-> GAS 通信) ==========
    FGameplayTag Event_Pickup_Start;     // 开始拾取
    FGameplayTag Event_Pickup_End;       // 拾取完成
    FGameplayTag Event_Photo_Start;      // 开始拍照
    FGameplayTag Event_Photo_End;        // 拍照完成
    FGameplayTag Event_Target_Found;     // 发现目标
    FGameplayTag Event_Target_Lost;      // 丢失目标
    FGameplayTag Event_Charge_Complete;  // 充电完成
    FGameplayTag Event_Patrol_WaypointReached;  // 到达巡逻点

    // ========== Status Tags (状态标记) ==========
    FGameplayTag Status_CanPickup;       // 可以拾取
    FGameplayTag Status_Holding;         // 正在持有物品
    FGameplayTag Status_Moving;          // 正在移动
    FGameplayTag Status_Patrolling;      // 正在巡逻
    FGameplayTag Status_Searching;       // 正在搜索
    FGameplayTag Status_Observing;       // 正在观察
    FGameplayTag Status_Charging;        // 正在充电
    FGameplayTag Status_InFormation;     // 编队中
    FGameplayTag Status_Avoiding;        // 避障中
    FGameplayTag Status_LowEnergy;       // 低电量

    // ========== Command Tags (外部命令) ==========
    FGameplayTag Command_Patrol;         // 命令: 开始巡逻
    FGameplayTag Command_Charge;         // 命令: 去充电
    FGameplayTag Command_Idle;           // 命令: 停止/空闲
    FGameplayTag Command_Search;         // 命令: 开始搜索

private:
    static FMAGameplayTags GameplayTags;
};
