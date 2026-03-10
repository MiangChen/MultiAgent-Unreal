// MASceneGraphRuntimeSyncUseCases.h
// SceneGraph 运行时同步用例 (P4a)

#pragma once

#include "CoreMinimal.h"
#include "Core/SceneGraph/Infrastructure/Ports/IMASceneGraphRuntimeSyncPort.h"

class AActor;
class UWorld;

class FMASceneGraphRuntimeSyncUseCases
{
public:
    static bool SyncNodePosition(
        IMASceneGraphRuntimeSyncPort& SyncPort,
        const FString& NodeId,
        const FVector& Position);

    static bool SyncNodeFeature(
        IMASceneGraphRuntimeSyncPort& SyncPort,
        const FString& NodeId,
        const FString& Key,
        const FString& Value);

    static void SyncRobotPosition(
        IMASceneGraphRuntimeSyncPort& SyncPort,
        const FString& RobotNodeId,
        const FVector& RobotLocation);

    static int32 SyncCarriedItemPositions(
        IMASceneGraphRuntimeSyncPort& SyncPort,
        const TArray<AActor*>& CarriedItems);

    static bool SyncActorPositionToNode(
        IMASceneGraphRuntimeSyncPort& SyncPort,
        UWorld* World,
        const FString& TargetNodeId,
        float PositionSyncThreshold);

    static int32 SyncDynamicNodePositions(
        IMASceneGraphRuntimeSyncPort& SyncPort,
        UWorld* World,
        float PositionSyncThreshold);

    static bool SyncRobotBatteryFeature(
        IMASceneGraphRuntimeSyncPort& SyncPort,
        const FMASceneGraphNode& Node,
        AActor* Actor,
        float BatterySyncTolerance = 1.f);

    static bool SyncPlacedItemState(
        IMASceneGraphRuntimeSyncPort& SyncPort,
        const FString& ItemNodeId,
        const FVector& ItemLocation,
        const FString& SupportNodeId);

    static bool MarkHazardExtinguished(
        IMASceneGraphRuntimeSyncPort& SyncPort,
        const FString& HazardNodeId);
};
