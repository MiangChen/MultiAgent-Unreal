// MASceneGraphUpdater.h
// 场景图更新器 - 负责将 UE 场景状态同步到场景图
//
// 两种更新环节:
// 1. 技能完成后更新 (兜底): 每个技能执行完后，更新相关节点
// 2. 周期性同步 (增量): 定时扫描动态节点，将位置差异同步到场景图
//
// 设计原则:
// - 单一职责: 只负责 UE 场景 → 场景图 的单向同步
// - 增量更新: 周期性同步只处理位置发生变化的节点，避免全量遍历
// - 统一入口: 技能完成后通过 UpdateAfterSkillCompletion 统一调用

#pragma once

#include "CoreMinimal.h"

class AMACharacter;
class UMASceneGraphManager;
class UWorld;
enum class EMACommand : uint8;

/**
 * 场景图更新器
 */
class MULTIAGENT_API FMASceneGraphUpdater
{
public:
    //=========================================================================
    // 技能完成后更新 (兜底)
    //=========================================================================

    /**
     * 技能完成后更新场景图（统一入口）
     * @param Agent 执行技能的 Agent
     * @param Command 技能类型
     * @param bSuccess 执行是否成功
     */
    static void UpdateAfterSkillCompletion(AMACharacter* Agent, EMACommand Command, bool bSuccess);

    //=========================================================================
    // 周期性同步 (增量)
    //=========================================================================

    /**
     * 增量同步所有动态节点
     * 遍历所有动态节点，对比 Actor 实际位置与节点记录位置，
     * 仅更新发生变化的节点。
     * 
     * @param World 世界指针
     * @return 本次同步更新的节点数量
     */
    static int32 SyncDynamicNodes(UWorld* World);

private:
    //=========================================================================
    // 辅助方法
    //=========================================================================

    static UMASceneGraphManager* GetSceneGraphManager(AMACharacter* Agent);
    static UMASceneGraphManager* GetSceneGraphManager(UWorld* World);

    /** 更新机器人节点位置 */
    static void UpdateRobotPosition(AMACharacter* Agent);

    /** 更新机器人携带物品的节点位置 (UGV 等) */
    static void UpdateCarriedItemPositions(AMACharacter* Agent);

    /**
     * 根据场景图节点 ID 同步目标 Actor 的位置到场景图
     * 通过节点的 GUID 找到 Actor，用 Actor 的实际位置更新节点
     * @param Agent 用于获取 World 和 SceneGraphManager
     * @param TargetNodeId 目标节点 ID
     */
    static void SyncActorPositionToNode(AMACharacter* Agent, const FString& TargetNodeId);

    //=========================================================================
    // 各技能的场景图更新
    //=========================================================================

    static void UpdateAfterNavigate(AMACharacter* Agent, bool bSuccess);
    static void UpdateAfterSearch(AMACharacter* Agent, bool bSuccess);
    static void UpdateAfterFollow(AMACharacter* Agent, bool bSuccess);
    static void UpdateAfterCharge(AMACharacter* Agent, bool bSuccess);
    static void UpdateAfterPlace(AMACharacter* Agent, bool bSuccess);
    static void UpdateAfterTakeOff(AMACharacter* Agent, bool bSuccess);
    static void UpdateAfterLand(AMACharacter* Agent, bool bSuccess);
    static void UpdateAfterReturnHome(AMACharacter* Agent, bool bSuccess);
    static void UpdateAfterHandleHazard(AMACharacter* Agent, bool bSuccess);
    static void UpdateAfterGuide(AMACharacter* Agent, bool bSuccess);

    /** 位置差异阈值 (cm)，低于此值不触发更新 */
    static constexpr float PositionSyncThreshold = 10.f;
};
