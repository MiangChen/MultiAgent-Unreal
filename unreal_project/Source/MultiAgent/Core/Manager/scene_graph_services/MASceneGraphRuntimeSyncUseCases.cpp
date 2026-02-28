// MASceneGraphRuntimeSyncUseCases.cpp

#include "scene_graph_services/MASceneGraphRuntimeSyncUseCases.h"
#include "../../../Agent/Skill/Utils/MAUESceneQuery.h"
#include "../../../Agent/Character/MACharacter.h"
#include "../../../Agent/Skill/MASkillComponent.h"
#include "GameFramework/Actor.h"

bool FMASceneGraphRuntimeSyncUseCases::SyncNodePosition(
    IMASceneGraphRuntimeSyncPort& SyncPort,
    const FString& NodeId,
    const FVector& Position)
{
    if (NodeId.IsEmpty())
    {
        return false;
    }

    SyncPort.UpdateDynamicNodePosition(NodeId, Position);
    return true;
}

bool FMASceneGraphRuntimeSyncUseCases::SyncNodeFeature(
    IMASceneGraphRuntimeSyncPort& SyncPort,
    const FString& NodeId,
    const FString& Key,
    const FString& Value)
{
    if (NodeId.IsEmpty() || Key.IsEmpty())
    {
        return false;
    }

    SyncPort.UpdateDynamicNodeFeature(NodeId, Key, Value);
    return true;
}

void FMASceneGraphRuntimeSyncUseCases::SyncRobotPosition(
    IMASceneGraphRuntimeSyncPort& SyncPort,
    const FString& RobotNodeId,
    const FVector& RobotLocation)
{
    SyncNodePosition(SyncPort, RobotNodeId, RobotLocation);
}

int32 FMASceneGraphRuntimeSyncUseCases::SyncCarriedItemPositions(
    IMASceneGraphRuntimeSyncPort& SyncPort,
    const TArray<AActor*>& CarriedItems)
{
    int32 UpdatedCount = 0;
    for (AActor* Item : CarriedItems)
    {
        if (!Item)
        {
            continue;
        }

        const FString ItemGuid = Item->GetActorGuid().ToString();
        const TArray<FMASceneGraphNode> Nodes = SyncPort.FindNodesByGuid(ItemGuid);
        const FVector ItemLocation = Item->GetActorLocation();

        for (const FMASceneGraphNode& Node : Nodes)
        {
            SyncPort.UpdateDynamicNodePosition(Node.Id, ItemLocation);
            ++UpdatedCount;
        }
    }
    return UpdatedCount;
}

bool FMASceneGraphRuntimeSyncUseCases::SyncActorPositionToNode(
    IMASceneGraphRuntimeSyncPort& SyncPort,
    UWorld* World,
    const FString& TargetNodeId,
    float PositionSyncThreshold)
{
    if (!World || TargetNodeId.IsEmpty())
    {
        return false;
    }

    const FMASceneGraphNode Node = SyncPort.GetNodeById(TargetNodeId);
    if (!Node.IsValid() || Node.Guid.IsEmpty())
    {
        return false;
    }

    AActor* TargetActor = FMAUESceneQuery::FindActorByGuid(World, Node.Guid);
    if (!TargetActor)
    {
        return false;
    }

    const FVector ActualPos = TargetActor->GetActorLocation();
    if (ActualPos.Equals(Node.Center, PositionSyncThreshold))
    {
        return false;
    }

    SyncPort.UpdateDynamicNodePosition(Node.Id, ActualPos);
    return true;
}

int32 FMASceneGraphRuntimeSyncUseCases::SyncDynamicNodePositions(
    IMASceneGraphRuntimeSyncPort& SyncPort,
    UWorld* World,
    float PositionSyncThreshold)
{
    if (!World)
    {
        return 0;
    }

    const TArray<FMASceneGraphNode> AllNodes = SyncPort.GetAllNodes();
    int32 UpdatedCount = 0;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (!Node.bIsDynamic || Node.Guid.IsEmpty())
        {
            continue;
        }

        AActor* Actor = FMAUESceneQuery::FindActorByGuid(World, Node.Guid);
        if (!Actor)
        {
            continue;
        }

        const FVector ActualPos = Actor->GetActorLocation();
        if (ActualPos.Equals(Node.Center, PositionSyncThreshold))
        {
            continue;
        }

        SyncPort.UpdateDynamicNodePosition(Node.Id, ActualPos);
        ++UpdatedCount;
    }

    return UpdatedCount;
}

bool FMASceneGraphRuntimeSyncUseCases::SyncRobotBatteryFeature(
    IMASceneGraphRuntimeSyncPort& SyncPort,
    const FMASceneGraphNode& Node,
    AActor* Actor,
    float BatterySyncTolerance)
{
    if (!Actor || Node.Category != TEXT("robot"))
    {
        return false;
    }

    AMACharacter* Character = Cast<AMACharacter>(Actor);
    if (!Character)
    {
        return false;
    }

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        return false;
    }

    const float CurrentPercent = SkillComp->GetEnergyPercent();
    float NodeBattery = 100.f;
    if (const FString* BatteryStr = Node.Features.Find(TEXT("battery_level")))
    {
        NodeBattery = FCString::Atof(**BatteryStr);
    }

    if (FMath::IsNearlyEqual(CurrentPercent, NodeBattery, BatterySyncTolerance))
    {
        return false;
    }

    SyncPort.UpdateDynamicNodeFeature(
        Node.Id,
        TEXT("battery_level"),
        FString::Printf(TEXT("%.0f"), CurrentPercent));
    return true;
}

bool FMASceneGraphRuntimeSyncUseCases::SyncPlacedItemState(
    IMASceneGraphRuntimeSyncPort& SyncPort,
    const FString& ItemNodeId,
    const FVector& ItemLocation,
    const FString& SupportNodeId)
{
    if (ItemNodeId.IsEmpty())
    {
        return false;
    }

    bool bUpdated = SyncNodePosition(SyncPort, ItemNodeId, ItemLocation);

    bool bPlacedOnRobot = false;
    if (!SupportNodeId.IsEmpty())
    {
        const FMASceneGraphNode SupportNode = SyncPort.GetNodeById(SupportNodeId);
        bPlacedOnRobot = SupportNode.IsValid() && SupportNode.IsRobot();
    }

    bUpdated = SyncNodeFeature(
        SyncPort,
        ItemNodeId,
        TEXT("is_carried"),
        bPlacedOnRobot ? TEXT("true") : TEXT("false")) || bUpdated;

    if (bPlacedOnRobot)
    {
        bUpdated = SyncNodeFeature(
            SyncPort,
            ItemNodeId,
            TEXT("carrier_id"),
            SupportNodeId) || bUpdated;
    }

    return bUpdated;
}

bool FMASceneGraphRuntimeSyncUseCases::MarkHazardExtinguished(
    IMASceneGraphRuntimeSyncPort& SyncPort,
    const FString& HazardNodeId)
{
    return SyncNodeFeature(SyncPort, HazardNodeId, TEXT("is_fire"), TEXT("false"));
}
