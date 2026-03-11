// MANavigationService.FollowGround.cpp
// Ground follow updates, navmesh refresh, and direct movement fallback

#include "MANavigationService.h"
#include "Agent/Navigation/Application/MANavigationUseCases.h"
#include "../Infrastructure/MAPathPlanner.h"
#include "AIController.h"
#include "GameFramework/Character.h"

void UMANavigationService::UpdateGroundFollowNavMesh(AAIController* AICtrl, const FVector& FollowPos, float CurrentTimeSeconds)
{
    const bool bShouldRefresh = FMANavigationUseCases::ShouldRefreshGroundFollowNav(
        !LastFollowTargetPos.IsZero(),
        FVector::Dist2D(FollowPos, LastFollowTargetPos),
        CurrentTimeSeconds,
        LastNavMeshNavigateTime,
        NavMeshFollowUpdateInterval);
    if (!AICtrl || !bShouldRefresh)
    {
        return;
    }

    LastFollowTargetPos = FollowPos;
    LastNavMeshNavigateTime = CurrentTimeSeconds;

    UE_LOG(LogTemp, Verbose, TEXT("[MANavigationService] %s: NavMesh follow update -> (%.0f, %.0f, %.0f)"),
        *OwnerCharacter->GetName(), FollowPos.X, FollowPos.Y, FollowPos.Z);

    AICtrl->MoveToLocation(FollowPos, AcceptanceRadius * 0.5f, true, true, false, true, nullptr);
}

void UMANavigationService::UpdateGroundFollowDirect(const FVector& CurrentLocation, const FVector& FollowPos, float DeltaTime)
{
    if (!OwnerCharacter)
    {
        return;
    }

    if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
    {
        AICtrl->StopMovement();
    }

    FVector MoveDirection = FVector::ZeroVector;
    if (PathPlanner.IsValid())
    {
        MoveDirection = PathPlanner->CalculateDirection(
            OwnerCharacter->GetWorld(),
            CurrentLocation,
            FollowPos,
            OwnerCharacter
        );
    }
    else
    {
        MoveDirection = (FollowPos - CurrentLocation).GetSafeNormal();
    }

    MoveDirection.Z = 0.f;
    OwnerCharacter->AddMovementInput(MoveDirection, 1.0f);

    if (!MoveDirection.IsNearlyZero())
    {
        FRotator TargetRotation = MoveDirection.Rotation();
        TargetRotation.Pitch = 0.f;
        TargetRotation.Roll = 0.f;
        const FRotator NewRotation = FMath::RInterpTo(
            OwnerCharacter->GetActorRotation(),
            TargetRotation,
            DeltaTime,
            5.f
        );
        OwnerCharacter->SetActorRotation(NewRotation);
    }
}

void UMANavigationService::UpdateFollowMode(float DeltaTime)
{
    if (!bIsFollowingActor || !OwnerCharacter || !FollowTarget.IsValid())
    {
        HandleFollowFailure(TEXT("Follow failed: Target lost"));
        return;
    }

    const FVector MyLoc = OwnerCharacter->GetActorLocation();
    const FVector TargetLoc = FollowTarget->GetActorLocation();
    const FVector FollowPos = CalculateFollowPosition();
    const float DistanceToFollowPos = FVector::Dist2D(MyLoc, FollowPos);
    const float DistanceToTarget = FVector::Dist2D(MyLoc, TargetLoc);

    // 已到达跟随位置
    if (DistanceToFollowPos < AcceptanceRadius)
    {
        // 停止 AIController 移动，避免抖动
        if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
        {
            AICtrl->StopMovement();
        }
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Arrived at follow position, waiting..."),
            *OwnerCharacter->GetName());
        return;  // 保持跟随状态，等待目标移动
    }

    AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController());
    const bool bUseNavMeshFollow = AICtrl && DistanceToTarget > NavMeshDistanceThreshold;
    if (bUseNavMeshFollow)
    {
        const float CurrentTimeSeconds = OwnerCharacter->GetWorld()
            ? OwnerCharacter->GetWorld()->GetTimeSeconds()
            : 0.f;
        UpdateGroundFollowNavMesh(AICtrl, FollowPos, CurrentTimeSeconds);
        return;
    }

    UpdateGroundFollowDirect(MyLoc, FollowPos, DeltaTime);
}
