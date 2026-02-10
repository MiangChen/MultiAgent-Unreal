// MASceneGraphUpdater.cpp
// 场景图更新器 - 将 UE 场景状态同步到场景图

#include "MASceneGraphUpdater.h"
#include "MAUESceneQuery.h"
#include "../../Character/MACharacter.h"
#include "../../Character/MAUGVCharacter.h"
#include "../MASkillComponent.h"
#include "../../../Core/Manager/MACommandManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Core/Manager/scene_graph_tools/MASceneGraphQuery.h"

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
    if (!World) return nullptr;
    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return nullptr;
    return GI->GetSubsystem<UMASceneGraphManager>();
}

void FMASceneGraphUpdater::UpdateRobotPosition(AMACharacter* Agent)
{
    if (!Agent) return;

    UMASceneGraphManager* SGM = GetSceneGraphManager(Agent);
    if (!SGM) return;

    SGM->UpdateDynamicNodePosition(Agent->AgentID, Agent->GetActorLocation());
}

void FMASceneGraphUpdater::UpdateCarriedItemPositions(AMACharacter* Agent)
{
    if (!Agent) return;

    AMAUGVCharacter* UGV = Cast<AMAUGVCharacter>(Agent);
    if (!UGV || !UGV->HasCargo()) return;

    UMASceneGraphManager* SGM = GetSceneGraphManager(Agent);
    if (!SGM) return;

    for (AActor* Item : UGV->CarriedItems)
    {
        if (!Item) continue;

        // 通过 Actor GUID 找到对应的场景图节点
        FString ItemGuid = Item->GetActorGuid().ToString();
        TArray<FMASceneGraphNode> Nodes = SGM->FindNodesByGuid(ItemGuid);

        for (const FMASceneGraphNode& Node : Nodes)
        {
            SGM->UpdateDynamicNodePosition(Node.Id, Item->GetActorLocation());
        }
    }
}

void FMASceneGraphUpdater::SyncActorPositionToNode(AMACharacter* Agent, const FString& TargetNodeId)
{
    if (!Agent || TargetNodeId.IsEmpty()) return;

    UMASceneGraphManager* SGM = GetSceneGraphManager(Agent);
    if (!SGM) return;

    // 获取目标节点，通过 GUID 找到 Actor，用 Actor 实际位置更新节点
    FMASceneGraphNode Node = SGM->GetNodeById(TargetNodeId);
    if (!Node.IsValid() || Node.Guid.IsEmpty()) return;

    AActor* TargetActor = FMAUESceneQuery::FindActorByGuid(Agent->GetWorld(), Node.Guid);
    if (!TargetActor) return;

    FVector ActualPos = TargetActor->GetActorLocation();
    if (!ActualPos.Equals(Node.Center, PositionSyncThreshold))
    {
        SGM->UpdateDynamicNodePosition(Node.Id, ActualPos);
        UE_LOG(LogMASceneGraphUpdater, Log, TEXT("Synced target node '%s' position to (%.0f, %.0f, %.0f)"),
            *Node.Id, ActualPos.X, ActualPos.Y, ActualPos.Z);
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

    // 更新物品位置
    SGM->UpdateDynamicNodePosition(Object1NodeId, Ctx.PlaceFinalLocation);

    // 判断是否放到了机器人上，更新携带状态
    bool bPlacedOnRobot = false;
    FString CarrierId;

    if (!Object2NodeId.IsEmpty())
    {
        FMASceneGraphNode Object2Node = SGM->GetNodeById(Object2NodeId);
        if (Object2Node.IsValid() && Object2Node.IsRobot())
        {
            bPlacedOnRobot = true;
            CarrierId = Object2NodeId;
        }
    }

    SGM->UpdateDynamicNodeFeature(Object1NodeId, TEXT("is_carried"), bPlacedOnRobot ? TEXT("true") : TEXT("false"));
    if (bPlacedOnRobot)
    {
        SGM->UpdateDynamicNodeFeature(Object1NodeId, TEXT("carrier_id"), CarrierId);
    }

    UE_LOG(LogMASceneGraphUpdater, Log, TEXT("Place: Item=%s → (%.0f, %.0f, %.0f), Carried=%s"),
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
            SGM->UpdateDynamicNodeFeature(Ctx.HazardTargetId, TEXT("is_fire"), TEXT("false"));
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

    TArray<FMASceneGraphNode> AllNodes = SGM->GetAllNodes();
    int32 UpdatedCount = 0;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (!Node.bIsDynamic || Node.Guid.IsEmpty()) continue;

        AActor* Actor = FMAUESceneQuery::FindActorByGuid(World, Node.Guid);
        if (!Actor) continue;

        // 位置同步
        FVector ActualPos = Actor->GetActorLocation();
        if (!ActualPos.Equals(Node.Center, PositionSyncThreshold))
        {
            SGM->UpdateDynamicNodePosition(Node.Id, ActualPos);
            UpdatedCount++;
        }

        // 电量同步 (SkillComponent → 场景图)
        if (Node.Category == TEXT("robot"))
        {
            if (AMACharacter* Character = Cast<AMACharacter>(Actor))
            {
                if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
                {
                    float CurrentPercent = SkillComp->GetEnergyPercent();
                    float NodeBattery = 100.f;
                    if (const FString* BatteryStr = Node.Features.Find(TEXT("battery_level")))
                    {
                        NodeBattery = FCString::Atof(**BatteryStr);
                    }
                    if (!FMath::IsNearlyEqual(CurrentPercent, NodeBattery, 1.f))
                    {
                        SGM->UpdateDynamicNodeFeature(Node.Id, TEXT("battery_level"),
                            FString::Printf(TEXT("%.0f"), CurrentPercent));
                        UpdatedCount++;
                    }
                }
            }
        }
    }

    if (UpdatedCount > 0)
    {
        UE_LOG(LogMASceneGraphUpdater, Log, TEXT("SyncDynamicNodes: Updated %d node(s)"), UpdatedCount);
    }

    return UpdatedCount;
}
