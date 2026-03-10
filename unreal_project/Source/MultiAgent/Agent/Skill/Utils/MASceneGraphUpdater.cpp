// MASceneGraphUpdater.cpp
// 场景图更新器 - 将 UE 场景状态同步到场景图

#include "MASceneGraphUpdater.h"
#include "MAUESceneQuery.h"
#include "../../Character/MACharacter.h"
#include "../../Character/MAUGVCharacter.h"
#include "../MASkillComponent.h"
#include "../../../Core/Command/Runtime/MACommandManager.h"
#include "../../../Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "../../../Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.h"
#include "../../../Core/SceneGraph/Infrastructure/Adapters/MASceneGraphRuntimeSyncAdapter.h"
#include "../../../Core/SceneGraph/Application/MASceneGraphRuntimeSyncUseCases.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneGraphUpdater, Log, All);

//=============================================================================
// 辅助方法
//=============================================================================

UMASceneGraphManager* FMASceneGraphUpdater::GetSceneGraphManager(AMACharacter* Agent)
{
    if (!Agent) return nullptr;
    return GetSceneGraphManager(Agent->GetWorld());
}

UMASceneGraphManager* FMASceneGraphUpdater::GetSceneGraphManager(UWorld* World)
{
    return World ? FMASceneGraphBootstrap::Resolve(World) : nullptr;
}

void FMASceneGraphUpdater::UpdateRobotPosition(AMACharacter* Agent)
{
    if (!Agent) return;

    UMASceneGraphManager* SGM = GetSceneGraphManager(Agent);
    if (!SGM) return;

    FMASceneGraphRuntimeSyncAdapter SyncAdapter(SGM);
    FMASceneGraphRuntimeSyncUseCases::SyncRobotPosition(
        SyncAdapter,
        Agent->AgentID,
        Agent->GetActorLocation());
}

void FMASceneGraphUpdater::UpdateCarriedItemPositions(AMACharacter* Agent)
{
    if (!Agent) return;

    AMAUGVCharacter* UGV = Cast<AMAUGVCharacter>(Agent);
    if (!UGV || !UGV->HasCargo()) return;

    UMASceneGraphManager* SGM = GetSceneGraphManager(Agent);
    if (!SGM) return;

    FMASceneGraphRuntimeSyncAdapter SyncAdapter(SGM);
    FMASceneGraphRuntimeSyncUseCases::SyncCarriedItemPositions(SyncAdapter, UGV->CarriedItems);
}

void FMASceneGraphUpdater::SyncActorPositionToNode(AMACharacter* Agent, const FString& TargetNodeId)
{
    if (!Agent || TargetNodeId.IsEmpty()) return;

    UMASceneGraphManager* SGM = GetSceneGraphManager(Agent);
    if (!SGM) return;

    FMASceneGraphRuntimeSyncAdapter SyncAdapter(SGM);
    if (FMASceneGraphRuntimeSyncUseCases::SyncActorPositionToNode(
        SyncAdapter,
        Agent->GetWorld(),
        TargetNodeId,
        PositionSyncThreshold))
    {
        UE_LOG(LogMASceneGraphUpdater, Log, TEXT("Synced target node '%s' to actor position"), *TargetNodeId);
    }
}

//=============================================================================
// 技能完成后更新 (统一入口)
//=============================================================================

void FMASceneGraphUpdater::UpdateAfterSkillCompletion(AMACharacter* Agent, EMACommand Command, bool bSuccess)
{
    if (!Agent) return;

    switch (Command)
    {
        case EMACommand::Navigate:     UpdateAfterNavigate(Agent, bSuccess);     break;
        case EMACommand::Search:       UpdateAfterSearch(Agent, bSuccess);       break;
        case EMACommand::Follow:       UpdateAfterFollow(Agent, bSuccess);       break;
        case EMACommand::Charge:       UpdateAfterCharge(Agent, bSuccess);       break;
        case EMACommand::Place:        UpdateAfterPlace(Agent, bSuccess);        break;
        case EMACommand::TakeOff:      UpdateAfterTakeOff(Agent, bSuccess);      break;
        case EMACommand::Land:         UpdateAfterLand(Agent, bSuccess);         break;
        case EMACommand::ReturnHome:   UpdateAfterReturnHome(Agent, bSuccess);   break;
        case EMACommand::HandleHazard: UpdateAfterHandleHazard(Agent, bSuccess); break;
        case EMACommand::Guide:        UpdateAfterGuide(Agent, bSuccess);        break;
        default: break;
    }
}

//=============================================================================
// 各技能的场景图更新
//=============================================================================

void FMASceneGraphUpdater::UpdateAfterNavigate(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess) return;

    UpdateRobotPosition(Agent);
    UpdateCarriedItemPositions(Agent);
}

void FMASceneGraphUpdater::UpdateAfterSearch(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess) return;

    UpdateRobotPosition(Agent);
}

void FMASceneGraphUpdater::UpdateAfterFollow(AMACharacter* Agent, bool bSuccess)
{
    // Follow 无论成功失败，robot 和目标都可能已移动
    UpdateRobotPosition(Agent);

    // 更新跟随目标的位置
    UMASkillComponent* SkillComp = Agent ? Agent->GetSkillComponent() : nullptr;
    if (!SkillComp) return;

    const FMAFeedbackContext& Ctx = SkillComp->GetFeedbackContext();
    if (!Ctx.FollowTargetId.IsEmpty())
    {
        SyncActorPositionToNode(Agent, Ctx.FollowTargetId);
    }
}

void FMASceneGraphUpdater::UpdateAfterCharge(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess) return;

    UpdateRobotPosition(Agent);
}

void FMASceneGraphUpdater::UpdateAfterPlace(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess || !Agent) return;

    UMASceneGraphManager* SGM = GetSceneGraphManager(Agent);
    if (!SGM) return;

    UpdateRobotPosition(Agent);

    UMASkillComponent* SkillComp = Agent->GetSkillComponent();
    if (!SkillComp) return;

    const FMAFeedbackContext& Ctx = SkillComp->GetFeedbackContext();
    FString Object1NodeId = Ctx.ObjectAttributes.FindRef(TEXT("object1_node_id"));
    FString Object2NodeId = Ctx.ObjectAttributes.FindRef(TEXT("object2_node_id"));

    // 兜底: 如果 object1_node_id 为空，尝试用 PlacedObjectName
    if (Object1NodeId.IsEmpty() && !Ctx.PlacedObjectName.IsEmpty())
    {
        Object1NodeId = Ctx.PlacedObjectName;
    }

    if (Object1NodeId.IsEmpty() || Ctx.PlaceFinalLocation.IsZero()) return;

    FMASceneGraphRuntimeSyncAdapter SyncAdapter(SGM);
    FMASceneGraphRuntimeSyncUseCases::SyncPlacedItemState(
        SyncAdapter,
        Object1NodeId,
        Ctx.PlaceFinalLocation,
        Object2NodeId);

    bool bPlacedOnRobot = false;
    if (!Object2NodeId.IsEmpty())
    {
        const FMASceneGraphNode Object2Node = SyncAdapter.GetNodeById(Object2NodeId);
        bPlacedOnRobot = Object2Node.IsValid() && Object2Node.IsRobot();
    }

    UE_LOG(LogMASceneGraphUpdater, Log, TEXT("Place: Item=%s -> (%.0f, %.0f, %.0f), Carried=%s"),
        *Object1NodeId, Ctx.PlaceFinalLocation.X, Ctx.PlaceFinalLocation.Y, Ctx.PlaceFinalLocation.Z,
        bPlacedOnRobot ? TEXT("true") : TEXT("false"));
}

void FMASceneGraphUpdater::UpdateAfterTakeOff(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess) return;
    UpdateRobotPosition(Agent);
}

void FMASceneGraphUpdater::UpdateAfterLand(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess) return;
    UpdateRobotPosition(Agent);
}

void FMASceneGraphUpdater::UpdateAfterReturnHome(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess) return;
    UpdateRobotPosition(Agent);
}

void FMASceneGraphUpdater::UpdateAfterHandleHazard(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess || !Agent) return;

    UpdateRobotPosition(Agent);

    UMASkillComponent* SkillComp = Agent->GetSkillComponent();
    if (!SkillComp) return;

    const FMAFeedbackContext& Ctx = SkillComp->GetFeedbackContext();
    if (!Ctx.HazardTargetId.IsEmpty())
    {
        UMASceneGraphManager* SGM = GetSceneGraphManager(Agent);
        if (SGM)
        {
            FMASceneGraphRuntimeSyncAdapter SyncAdapter(SGM);
            FMASceneGraphRuntimeSyncUseCases::MarkHazardExtinguished(SyncAdapter, Ctx.HazardTargetId);
            UE_LOG(LogMASceneGraphUpdater, Log, TEXT("HandleHazard: %s is_fire=false"), *Ctx.HazardTargetId);
        }
    }
}

void FMASceneGraphUpdater::UpdateAfterGuide(AMACharacter* Agent, bool bSuccess)
{
    // Guide 无论成功失败，robot 和被引导目标都可能已移动
    UpdateRobotPosition(Agent);

    // 更新被引导目标的位置
    UMASkillComponent* SkillComp = Agent ? Agent->GetSkillComponent() : nullptr;
    if (!SkillComp) return;

    const FMAFeedbackContext& Ctx = SkillComp->GetFeedbackContext();
    if (!Ctx.GuideTargetId.IsEmpty())
    {
        SyncActorPositionToNode(Agent, Ctx.GuideTargetId);
    }
}

//=============================================================================
// 周期性增量同步
//=============================================================================

int32 FMASceneGraphUpdater::SyncDynamicNodes(UWorld* World)
{
    if (!World) return 0;

    UMASceneGraphManager* SGM = GetSceneGraphManager(World);
    if (!SGM) return 0;

    FMASceneGraphRuntimeSyncAdapter SyncAdapter(SGM);
    TArray<FMASceneGraphNode> AllNodes = SGM->GetAllNodes();
    int32 UpdatedCount = FMASceneGraphRuntimeSyncUseCases::SyncDynamicNodePositions(
        SyncAdapter,
        World,
        PositionSyncThreshold);

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        // 电量同步 (SkillComponent → 场景图)
        if (Node.Category == TEXT("robot"))
        {
            if (Node.Guid.IsEmpty()) continue;

            AActor* Actor = FMAUESceneQuery::FindActorByGuid(World, Node.Guid);
            if (!Actor) continue;

            if (FMASceneGraphRuntimeSyncUseCases::SyncRobotBatteryFeature(
                SyncAdapter,
                Node,
                Actor,
                1.f))
            {
                UpdatedCount++;
            }
        }
    }

    if (UpdatedCount > 0)
    {
        UE_LOG(LogMASceneGraphUpdater, Log, TEXT("SyncDynamicNodes: Updated %d node(s)"), UpdatedCount);
    }

    return UpdatedCount;
}
