// MAUESceneApplier.cpp
// UE 场景应用器 - 将场景图节点变更同步到 UE 场景中的 Actor

#include "MAUESceneApplier.h"
#include "../MASceneGraphManager.h"
#include "../scene_graph_tools/MADynamicNodeManager.h"
#include "../../../Agent/Character/MACharacter.h"
#include "../../../Agent/Skill/MASkillComponent.h"
#include "../../../Agent/Component/MANavigationService.h"
#include "../../../Agent/Skill/Utils/MAUESceneQuery.h"
#include "../../../Environment/IMAEnvironmentObject.h"
#include "Components/CapsuleComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAUESceneApplier, Log, All);

//=============================================================================
// 内部辅助
//=============================================================================

AActor* FMAUESceneApplier::FindActorByGuid(UWorld* World, const FString& Guid)
{
    return FMAUESceneQuery::FindActorByGuid(World, Guid);
}

//=============================================================================
// 地面投影
//=============================================================================

float FMAUESceneApplier::CalculateGroundOffset(AActor* Actor)
{
    if (!Actor)
    {
        return 5.f; // 默认偏移
    }

    // MACharacter: 使用胶囊体半高
    if (AMACharacter* Character = Cast<AMACharacter>(Actor))
    {
        if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
        {
            return Capsule->GetScaledCapsuleHalfHeight();
        }
        return 90.f; // Character 默认半高
    }

    // 其他 Actor: 计算边界框底部到原点的距离
    FVector Origin, BoxExtent;
    Actor->GetActorBounds(false, Origin, BoxExtent);
    FVector ActorLocation = Actor->GetActorLocation();
    
    // 底部偏移 = Actor原点Z - 边界框底部Z
    float BottomZ = Origin.Z - BoxExtent.Z;
    float Offset = ActorLocation.Z - BottomZ;
    
    // 确保偏移为正值且合理
    return FMath::Max(Offset, 5.f);
}

FVector FMAUESceneApplier::SnapToGround(UWorld* World, const FVector& Location, float HeightOffset)
{
    if (!World)
    {
        return Location;
    }

    FHitResult HitResult;
    FVector TraceStart = FVector(Location.X, Location.Y, 10000.f);
    FVector TraceEnd = FVector(Location.X, Location.Y, -20000.f);

    if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
    {
        FVector Snapped = FVector(Location.X, Location.Y, HitResult.Location.Z + HeightOffset);
        UE_LOG(LogMAUESceneApplier, Verbose, TEXT("SnapToGround: [%.0f, %.0f] Z: %.0f → %.0f (offset=%.1f)"),
            Location.X, Location.Y, Location.Z, Snapped.Z, HeightOffset);
        return Snapped;
    }

    UE_LOG(LogMAUESceneApplier, Warning, TEXT("SnapToGround: No ground at [%.0f, %.0f], keeping Z=%.0f"),
        Location.X, Location.Y, Location.Z);
    return Location;
}

FVector FMAUESceneApplier::SnapActorToGround(UWorld* World, AActor* Actor, const FVector& Location)
{
    float Offset = CalculateGroundOffset(Actor);
    return SnapToGround(World, Location, Offset);
}

//=============================================================================
// 统一入口
//=============================================================================

int32 FMAUESceneApplier::ApplyNodeToScene(UWorld* World, const FMASceneGraphNode& Node)
{
    if (!World)
    {
        UE_LOG(LogMAUESceneApplier, Warning, TEXT("ApplyNodeToScene: World is null"));
        return 0;
    }

    int32 AppliedCount = 0;

    // point 类型: 单个 Actor (通过 Guid 字段)
    if (Node.ShapeType == TEXT("point") && !Node.Guid.IsEmpty())
    {
        AActor* Actor = FindActorByGuid(World, Node.Guid);
        if (Actor && ApplyNodeToActor(Actor, Node, World))
        {
            AppliedCount++;
        }
    }

    // polygon/linestring/prism 类型: 多个 Actor (通过 GuidArray)
    if (Node.GuidArray.Num() > 0)
    {
        TArray<AActor*> Actors = FMAUESceneQuery::FindActorsByGuidArray(World, Node.GuidArray);
        for (AActor* Actor : Actors)
        {
            if (Actor && ApplyNodeToActor(Actor, Node, World))
            {
                AppliedCount++;
            }
        }
    }

    UE_LOG(LogMAUESceneApplier, Log, TEXT("ApplyNodeToScene: Node %s (%s) → applied to %d actor(s)"),
        *Node.Id, *Node.Label, AppliedCount);

    return AppliedCount;
}

//=============================================================================
// 核心: 将节点属性应用到单个 Actor
//=============================================================================

bool FMAUESceneApplier::ApplyNodeToActor(AActor* Actor, const FMASceneGraphNode& Node, UWorld* World)
{
    if (!Actor)
    {
        return false;
    }

    bool bChanged = false;

    // --- 位置同步 (仅 point 类型，且 Center 非零) ---
    if (Node.ShapeType == TEXT("point") && !Node.Center.IsZero())
    {
        FVector CurrentLocation = Actor->GetActorLocation();
        // 只比较 XY，因为 Z 会被地面投影覆盖
        if (!FVector2D(CurrentLocation).Equals(FVector2D(Node.Center), 1.0f))
        {
            // 飞行机器人：保持当前高度，仅更新 XY；地面对象：地面投影
            bool bIsFlying = false;
            if (AMACharacter* Character = Cast<AMACharacter>(Actor))
            {
                if (UMANavigationService* NavService = Character->GetNavigationService())
                {
                    bIsFlying = NavService->bIsFlying;
                }
            }
            
            FVector NewLocation;
            if (bIsFlying)
            {
                NewLocation = FVector(Node.Center.X, Node.Center.Y, CurrentLocation.Z);
            }
            else
            {
                NewLocation = SnapActorToGround(World, Actor, Node.Center);
            }
            
            Actor->SetActorLocation(NewLocation);
            bChanged = true;
            UE_LOG(LogMAUESceneApplier, Log, TEXT("  Position: %s → [%.0f, %.0f, %.0f]%s"),
                *Actor->GetName(), NewLocation.X, NewLocation.Y, NewLocation.Z,
                bIsFlying ? TEXT(" (flying, kept Z)") : TEXT(" (snapped)"));
        }
    }

    // --- 旋转同步 (仅动态节点) ---
    if (Node.bIsDynamic && !Node.Rotation.IsZero())
    {
        FRotator CurrentRotation = Actor->GetActorRotation();
        if (!CurrentRotation.Equals(Node.Rotation, 1.0f))
        {
            Actor->SetActorRotation(Node.Rotation);
            bChanged = true;
            UE_LOG(LogMAUESceneApplier, Log, TEXT("  Rotation: %s → [%.1f, %.1f, %.1f]"),
                *Actor->GetName(), Node.Rotation.Pitch, Node.Rotation.Yaw, Node.Rotation.Roll);
        }
    }

    // --- 标签同步 ---
    if (!Node.Label.IsEmpty())
    {
        if (AMACharacter* Character = Cast<AMACharacter>(Actor))
        {
            if (Character->AgentLabel != Node.Label)
            {
                Character->AgentLabel = Node.Label;
                bChanged = true;
                UE_LOG(LogMAUESceneApplier, Log, TEXT("  AgentLabel: %s → %s"),
                    *Actor->GetName(), *Node.Label);
            }
        }
    }

    // --- 电量同步 (场景图 → SkillComponent) ---
    if (Node.Category == TEXT("robot"))
    {
        if (const FString* BatteryStr = Node.Features.Find(TEXT("battery_level")))
        {
            if (AMACharacter* Character = Cast<AMACharacter>(Actor))
            {
                if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
                {
                    float NodeBattery = FCString::Atof(**BatteryStr);
                    float CurrentPercent = SkillComp->GetEnergyPercent();
                    if (!FMath::IsNearlyEqual(NodeBattery, CurrentPercent, 1.f))
                    {
                        SkillComp->SetEnergy(SkillComp->MaxEnergy * NodeBattery / 100.f);
                        bChanged = true;
                        UE_LOG(LogMAUESceneApplier, Log, TEXT("  BatteryLevel: %s → %.0f%%"),
                            *Actor->GetName(), NodeBattery);
                    }
                }
            }
        }
    }

    return bChanged;
}

//=============================================================================
// 细粒度接口
//=============================================================================

bool FMAUESceneApplier::ApplyPosition(UWorld* World, const FString& ActorGuid, const FVector& NewLocation)
{
    if (!World || ActorGuid.IsEmpty())
    {
        return false;
    }

    AActor* Actor = FindActorByGuid(World, ActorGuid);
    if (!Actor)
    {
        UE_LOG(LogMAUESceneApplier, Warning, TEXT("ApplyPosition: Actor not found for GUID %s"), *ActorGuid);
        return false;
    }

    // 飞行机器人：保持当前高度，仅更新 XY
    bool bIsFlying = false;
    if (AMACharacter* Character = Cast<AMACharacter>(Actor))
    {
        if (UMANavigationService* NavService = Character->GetNavigationService())
        {
            bIsFlying = NavService->bIsFlying;
        }
    }

    FVector FinalLocation;
    if (bIsFlying)
    {
        FinalLocation = FVector(NewLocation.X, NewLocation.Y, Actor->GetActorLocation().Z);
    }
    else
    {
        FinalLocation = SnapActorToGround(World, Actor, NewLocation);
    }

    Actor->SetActorLocation(FinalLocation);
    UE_LOG(LogMAUESceneApplier, Log, TEXT("ApplyPosition: %s → [%.0f, %.0f, %.0f]%s"),
        *Actor->GetName(), FinalLocation.X, FinalLocation.Y, FinalLocation.Z,
        bIsFlying ? TEXT(" (flying, kept Z)") : TEXT(" (snapped)"));
    return true;
}

bool FMAUESceneApplier::ApplyRotation(UWorld* World, const FString& ActorGuid, const FRotator& NewRotation)
{
    if (!World || ActorGuid.IsEmpty())
    {
        return false;
    }

    AActor* Actor = FindActorByGuid(World, ActorGuid);
    if (!Actor)
    {
        UE_LOG(LogMAUESceneApplier, Warning, TEXT("ApplyRotation: Actor not found for GUID %s"), *ActorGuid);
        return false;
    }

    Actor->SetActorRotation(NewRotation);
    UE_LOG(LogMAUESceneApplier, Log, TEXT("ApplyRotation: %s → [%.1f, %.1f, %.1f]"),
        *Actor->GetName(), NewRotation.Pitch, NewRotation.Yaw, NewRotation.Roll);
    return true;
}

bool FMAUESceneApplier::ApplyLabel(UWorld* World, const FString& ActorGuid, const FString& NewLabel)
{
    if (!World || ActorGuid.IsEmpty() || NewLabel.IsEmpty())
    {
        return false;
    }

    AActor* Actor = FindActorByGuid(World, ActorGuid);
    if (!Actor)
    {
        UE_LOG(LogMAUESceneApplier, Warning, TEXT("ApplyLabel: Actor not found for GUID %s"), *ActorGuid);
        return false;
    }

    if (AMACharacter* Character = Cast<AMACharacter>(Actor))
    {
        Character->AgentLabel = NewLabel;
        UE_LOG(LogMAUESceneApplier, Log, TEXT("ApplyLabel: %s AgentLabel → %s"), *Actor->GetName(), *NewLabel);
        return true;
    }

    Actor->SetActorLabel(NewLabel);
    UE_LOG(LogMAUESceneApplier, Log, TEXT("ApplyLabel: %s ActorLabel → %s"), *Actor->GetName(), *NewLabel);
    return true;
}

//=============================================================================
// 差异应用
//=============================================================================

int32 FMAUESceneApplier::ApplyNodeDiff(UWorld* World, const FMASceneGraphNode& OldNode, const FMASceneGraphNode& NewNode)
{
    if (!World)
    {
        return 0;
    }

    int32 ChangeCount = 0;
    FString PrimaryGuid = NewNode.Guid.IsEmpty() ? OldNode.Guid : NewNode.Guid;

    if (NewNode.ShapeType == TEXT("point") && !PrimaryGuid.IsEmpty())
    {
        if (!OldNode.Center.Equals(NewNode.Center, 1.0f))
        {
            if (ApplyPosition(World, PrimaryGuid, NewNode.Center))
            {
                ChangeCount++;
            }
        }
    }

    if (NewNode.bIsDynamic && !PrimaryGuid.IsEmpty())
    {
        if (!OldNode.Rotation.Equals(NewNode.Rotation, 1.0f))
        {
            if (ApplyRotation(World, PrimaryGuid, NewNode.Rotation))
            {
                ChangeCount++;
            }
        }
    }

    if (!PrimaryGuid.IsEmpty() && OldNode.Label != NewNode.Label && !NewNode.Label.IsEmpty())
    {
        if (ApplyLabel(World, PrimaryGuid, NewNode.Label))
        {
            ChangeCount++;
        }
    }

    if (ChangeCount > 0)
    {
        UE_LOG(LogMAUESceneApplier, Log, TEXT("ApplyNodeDiff: Node %s → %d change(s) applied"),
            *NewNode.Id, ChangeCount);
    }

    return ChangeCount;
}

//=============================================================================
// 场景图校准 (Actor → 场景图)
//=============================================================================

bool FMAUESceneApplier::CalibrateNodeFromActor(UWorld* World, FMASceneGraphNode& Node)
{
    if (!World || Node.Guid.IsEmpty())
    {
        return false;
    }

    AActor* Actor = FindActorByGuid(World, Node.Guid);
    if (!Actor)
    {
        return false;
    }

    FVector ActualLocation = Actor->GetActorLocation();

    // 只在位置有差异时更新
    if (Node.Center.Equals(ActualLocation, 1.0f))
    {
        return false;
    }

    FVector OldCenter = Node.Center;
    Node.Center = ActualLocation;

    // 重新生成 RawJson 以反映新的 Center
    Node.RawJson = FMADynamicNodeManager::GenerateRawJson(Node);

    UE_LOG(LogMAUESceneApplier, Log, TEXT("CalibrateNodeFromActor: %s (%s) [%.0f, %.0f, %.0f] → [%.0f, %.0f, %.0f]"),
        *Node.Id, *Node.Label,
        OldCenter.X, OldCenter.Y, OldCenter.Z,
        ActualLocation.X, ActualLocation.Y, ActualLocation.Z);

    return true;
}
