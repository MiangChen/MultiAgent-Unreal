// MANavigationService.Follow.cpp
// 跟随模式相关实现

#include "MANavigationService.h"
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

bool UMANavigationService::FollowActor(AActor* Target, float InFollowDistance, float InAcceptanceRadius)
{
    if (!OwnerCharacter || !Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] FollowActor failed: Invalid owner or target"));
        return false;
    }
    
    // 停止之前的导航/跟随
    if (bIsFollowingActor) StopFollowing();
    if (CurrentState == EMANavigationState::Navigating) CancelNavigation();
    
    // 初始化跟随参数
    FollowTarget = Target;
    FollowDistance = InFollowDistance;
    AcceptanceRadius = InAcceptanceRadius;
    bIsFollowingActor = true;
    LastNavMeshNavigateTime = 0.f;
    LastFollowTargetPos = FVector::ZeroVector;
    
    // 初始化路径规划器（用于地面近距离避障）
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
    
    SetNavigationState(EMANavigationState::Navigating);
    StartFollowUpdateTimer();
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Started following %s (distance=%.0f, flying=%s)"),
        *OwnerCharacter->GetName(), *Target->GetName(), FollowDistance, bIsFlying ? TEXT("true") : TEXT("false"));
    
    return true;
}

void UMANavigationService::StopFollowing()
{
    if (!bIsFollowingActor) return;
    
    bIsFollowingActor = false;
    FollowTarget.Reset();
    LastFollowTargetPos = FVector::ZeroVector;
    LastNavMeshNavigateTime = 0.f;
    
    // 清理跟随定时器
    if (UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr)
    {
        World->GetTimerManager().ClearTimer(FollowModeTimerHandle);
        FollowModeTimerHandle.Invalidate();
    }
    
    // 停止飞行控制器
    if (FlightController.IsValid())
    {
        FlightController->CancelFlight();
        FlightController->StopMovement();
    }
    
    // 停止 AIController 移动
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
    if (!bIsFollowingActor || !OwnerCharacter || !FollowTarget.IsValid())
    {
        HandleFollowFailure(TEXT("Follow failed: Target lost"));
        return;
    }
    
    FVector FollowPos = CalculateFollowPosition();
    FVector MyLoc = OwnerCharacter->GetActorLocation();
    float DistanceToFollowPos = FVector::Dist(MyLoc, FollowPos);
    
    // 如果已经在跟随位置附近，只做轻微调整，避免抖动
    if (DistanceToFollowPos < AcceptanceRadius)
    {
        // 在容差范围内，只更新朝向，不移动
        FVector DirToTarget = (FollowTarget->GetActorLocation() - MyLoc).GetSafeNormal2D();
        if (!DirToTarget.IsNearlyZero())
        {
            FRotator TargetRot = DirToTarget.Rotation();
            TargetRot.Pitch = 0.f;
            TargetRot.Roll = 0.f;
            FRotator NewRot = FMath::RInterpTo(OwnerCharacter->GetActorRotation(), TargetRot, DeltaTime, 2.f);
            OwnerCharacter->SetActorRotation(NewRot);
        }
        return;
    }
    
    // 使用飞行控制器
    if (FlightController.IsValid())
    {
        FlightController->UpdateFollowTarget(FollowPos);
        FlightController->UpdateFlight(DeltaTime);
    }
    else
    {
        // 回退：简单直线跟随
        FVector MoveDir = (FollowPos - MyLoc).GetSafeNormal();
        
        OwnerCharacter->AddMovementInput(MoveDir, 1.0f);
        
        if (!MoveDir.IsNearlyZero())
        {
            FRotator TargetRot = MoveDir.Rotation();
            FRotator NewRot = FMath::RInterpTo(OwnerCharacter->GetActorRotation(), TargetRot, DeltaTime, 5.f);
            OwnerCharacter->SetActorRotation(NewRot);
        }
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
