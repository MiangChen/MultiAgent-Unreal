// MANavigationService.Follow.cpp
// 跟随模式相关实现

#include "MANavigationService.h"
#include "Agent/Navigation/Application/MANavigationUseCases.h"
#include "../Infrastructure/MAFlightController.h"
#include "AIController.h"
#include "GameFramework/Character.h"

void UMANavigationService::StartFollowUpdateTimer()
{
    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
    if (!World)
    {
        return;
    }

    if (FollowModeTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(FollowModeTimerHandle);
    }

    const auto UpdateFunction = bIsFlying
        ? &UMANavigationService::UpdateFlightFollowMode
        : &UMANavigationService::UpdateFollowMode;
    World->GetTimerManager().SetTimer(
        FollowModeTimerHandle,
        FTimerDelegate::CreateUObject(this, UpdateFunction, FollowModeUpdateInterval),
        FollowModeUpdateInterval,
        true
    );
}

void UMANavigationService::HandleFollowFailure(const TCHAR* FailureReason)
{
    StopFollowing();
    SetNavigationState(EMANavigationState::Failed);
    OnNavigationCompleted.Broadcast(false, FailureReason);
}

void UMANavigationService::InitializeFollowSession(AActor& Target, const float InFollowDistance, const float InAcceptanceRadius)
{
    FollowTarget = &Target;
    FollowDistance = InFollowDistance;
    AcceptanceRadius = InAcceptanceRadius;
    bIsFollowingActor = true;
    LastNavMeshNavigateTime = 0.f;
    LastFollowTargetPos = FVector::ZeroVector;
}

void UMANavigationService::CleanupFollowSession()
{
    bIsFollowingActor = false;
    FollowTarget.Reset();
    LastFollowTargetPos = FVector::ZeroVector;
    LastNavMeshNavigateTime = 0.f;

    if (UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr)
    {
        World->GetTimerManager().ClearTimer(FollowModeTimerHandle);
        FollowModeTimerHandle.Invalidate();
    }
}

void UMANavigationService::RotateOwnerToward(const FVector& DesiredLocation, const float DeltaTime, const float InterpSpeed)
{
    if (!OwnerCharacter)
    {
        return;
    }

    const FVector DirToTarget = (DesiredLocation - OwnerCharacter->GetActorLocation()).GetSafeNormal2D();
    if (DirToTarget.IsNearlyZero())
    {
        return;
    }

    FRotator TargetRot = DirToTarget.Rotation();
    TargetRot.Pitch = 0.f;
    TargetRot.Roll = 0.f;
    const FRotator NewRot = FMath::RInterpTo(OwnerCharacter->GetActorRotation(), TargetRot, DeltaTime, InterpSpeed);
    OwnerCharacter->SetActorRotation(NewRot);
}

void UMANavigationService::ApplyFallbackFlightFollowMovement(
    const FVector& FollowPos,
    const FVector& CurrentLocation,
    const float DeltaTime)
{
    if (!OwnerCharacter)
    {
        return;
    }

    const FVector MoveDir = (FollowPos - CurrentLocation).GetSafeNormal();
    OwnerCharacter->AddMovementInput(MoveDir, 1.0f);

    if (!MoveDir.IsNearlyZero())
    {
        FRotator TargetRot = MoveDir.Rotation();
        const FRotator NewRot = FMath::RInterpTo(OwnerCharacter->GetActorRotation(), TargetRot, DeltaTime, 5.f);
        OwnerCharacter->SetActorRotation(NewRot);
    }
}

bool UMANavigationService::FollowActor(AActor* Target, float InFollowDistance, float InAcceptanceRadius)
{
    const FMANavigationFollowLifecycleFeedback Feedback =
        FMANavigationUseCases::BuildFollowStartLifecycle(
            OwnerCharacter != nullptr,
            Target != nullptr,
            bIsFollowingActor,
            CurrentState);
    if (!Feedback.bCanStart)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s"), *Feedback.FailureMessage);
        return false;
    }

    if (Feedback.bShouldStopPreviousFollow)
    {
        StopFollowing();
    }
    if (Feedback.bShouldCancelActiveNavigation)
    {
        CancelNavigation();
    }

    InitializeFollowSession(*Target, InFollowDistance, InAcceptanceRadius);

    if (!bIsFlying && !PathPlanner.IsValid())
    {
        PathPlanner = MakeUnique<FMAMultiLayerRaycast>(PathPlannerConfig);
    }

    if (bIsFlying)
    {
        EnsureFlightControllerInitialized();
        if (FlightController.IsValid())
        {
            FlightController->SetAcceptanceRadius(AcceptanceRadius);
        }
    }
    
    SetNavigationState(Feedback.NextState);
    StartFollowUpdateTimer();
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Started following %s (distance=%.0f, flying=%s)"),
        *OwnerCharacter->GetName(), *Target->GetName(), FollowDistance, bIsFlying ? TEXT("true") : TEXT("false"));
    
    return true;
}

void UMANavigationService::StopFollowing()
{
    if (!bIsFollowingActor)
    {
        return;
    }

    CleanupFollowSession();

    if (FlightController.IsValid())
    {
        FlightController->CancelFlight();
        FlightController->StopMovement();
    }

    if (OwnerCharacter)
    {
        if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
        {
            AICtrl->StopMovement();
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] StopFollowing"));
}

void UMANavigationService::UpdateFlightFollowMode(float DeltaTime)
{
    const FVector FollowPos = CalculateFollowPosition();
    const FVector MyLoc = OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;
    const float DistanceToFollowPos = FVector::Dist(MyLoc, FollowPos);
    const FMANavigationFollowUpdateFeedback Feedback =
        FMANavigationUseCases::BuildFlightFollowUpdate(
            bIsFollowingActor,
            OwnerCharacter != nullptr,
            FollowTarget.IsValid(),
            DistanceToFollowPos,
            AcceptanceRadius);

    if (Feedback.Action == EMANavigationFollowUpdateAction::HandleLostTarget)
    {
        HandleFollowFailure(*Feedback.FailureMessage);
        return;
    }

    if (Feedback.Action == EMANavigationFollowUpdateAction::RotateOnly)
    {
        RotateOwnerToward(FollowTarget->GetActorLocation(), DeltaTime, 2.f);
        return;
    }

    if (FlightController.IsValid())
    {
        FlightController->UpdateFollowTarget(FollowPos);
        FlightController->UpdateFlight(DeltaTime);
    }
    else
    {
        ApplyFallbackFlightFollowMovement(FollowPos, MyLoc, DeltaTime);
    }
}

FVector UMANavigationService::CalculateFollowPosition() const
{
    if (!FollowTarget.IsValid() || !OwnerCharacter) return FVector::ZeroVector;
    
    FVector TargetLoc = FollowTarget->GetActorLocation();
    FVector MyLoc = OwnerCharacter->GetActorLocation();
    
    FVector FollowPos;
    
    // 计算跟随方向：使用目标的反向前向（在目标身后跟随）
    // 而不是从目标指向机器人的方向（会导致机器人永远追不上）
    FVector TargetForward = FollowTarget->GetActorForwardVector().GetSafeNormal2D();
    FVector FollowDir = -TargetForward;  // 目标身后的方向
    
    // 如果目标没有明确的前向（静止），则使用从目标指向机器人的方向
    if (FollowDir.IsNearlyZero())
    {
        FollowDir = (MyLoc - TargetLoc).GetSafeNormal2D();
        if (FollowDir.IsNearlyZero())
        {
            FollowDir = FVector::ForwardVector;
        }
    }
    
    if (bIsFlying)
    {
        // 飞行机器人：XY 平面上的跟随位置
        FollowPos = TargetLoc + FollowDir * FollowDistance;
        // 高度固定在目标上方 MinFlightAltitude
        FollowPos.Z = FMath::Max(TargetLoc.Z + MinFlightAltitude, MinFlightAltitude);
    }
    else
    {
        // 地面机器人
        FollowPos = TargetLoc + FollowDir * FollowDistance;
        FollowPos.Z = TargetLoc.Z;
    }
    
    return FollowPos;
}

FVector UMANavigationService::GetCurrentFollowPosition() const
{
    return CalculateFollowPosition();
}
