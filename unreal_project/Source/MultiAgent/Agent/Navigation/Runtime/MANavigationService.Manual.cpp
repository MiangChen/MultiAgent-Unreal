// MANavigationService.Manual.cpp
// Ground manual fallback navigation and local movement updates

#include "MANavigationService.h"
#include "Agent/Navigation/Application/MANavigationUseCases.h"
#include "../Infrastructure/MAPathPlanner.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"

void UMANavigationService::StartManualNavigation()
{
    if (!OwnerCharacter)
    {
        CompleteNavigation(false, TEXT("Navigate failed: Character lost"));
        return;
    }

    // 确保停止 AIController 的 NavMesh 寻路，避免与手动导航双重控制冲突
    if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
    {
        AICtrl->StopMovement();
        if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
        {
            PathComp->OnRequestFinished.RemoveAll(this);
        }
    }
    bHasActiveNavMeshRequest = false;

    bUsingManualNavigation = true;
    ManualNavStuckTime = 0.f;
    LastManualNavLocation = OwnerCharacter->GetActorLocation();

    // 根据配置选择路径规划算法
    if (PathPlannerType == EMAPathPlannerType::ElevationMap)
    {
        PathPlanner = MakeUnique<FMAElevationMapPathfinder>(PathPlannerConfig);
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Using ElevationMap pathfinder"),
            *OwnerCharacter->GetName());
    }
    else
    {
        PathPlanner = MakeUnique<FMAMultiLayerRaycast>(PathPlannerConfig);
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Using MultiLayerRaycast pathfinder"),
            *OwnerCharacter->GetName());
    }
    PathPlanner->Reset();

    // 启动手动导航更新定时器
    if (UWorld* World = OwnerCharacter->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ManualNavTimerHandle,
            FTimerDelegate::CreateUObject(this, &UMANavigationService::UpdateManualNavigation, ManualNavUpdateInterval),
            ManualNavUpdateInterval,
            true
        );
    }
}

void UMANavigationService::UpdateManualNavigation(float DeltaTime)
{
    if (!OwnerCharacter)
    {
        CompleteNavigation(false, TEXT("Navigate failed: Character lost during manual navigation"));
        return;
    }

    FVector CurrentLocation = OwnerCharacter->GetActorLocation();
    float Distance2D = FVector::Dist2D(CurrentLocation, TargetLocation);

    const float MovedDistance = FVector::Dist(CurrentLocation, LastManualNavLocation);
    const FMANavigationManualUpdateFeedback Feedback =
        FMANavigationUseCases::BuildManualNavigationUpdate(
            Distance2D,
            MovedDistance,
            DeltaTime,
            ManualNavStuckTime,
            AcceptanceRadius,
            StuckTimeout,
            TargetLocation);

    if (Feedback.Action == EMANavigationManualUpdateAction::CompleteSuccess)
    {
        SetNavigationState(EMANavigationState::Arrived);
        CompleteNavigation(true, Feedback.CompletionMessage);
        return;
    }

    if (Feedback.Action == EMANavigationManualUpdateAction::CompleteFailure)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: Manual navigation stuck for %.0fs, giving up"),
            *OwnerCharacter->GetName(), StuckTimeout);
        SetNavigationState(EMANavigationState::Failed);
        CompleteNavigation(false, Feedback.CompletionMessage);
        return;
    }

    ManualNavStuckTime = Feedback.NextStuckTime;
    if (Feedback.bShouldUpdateLastLocation)
    {
        LastManualNavLocation = CurrentLocation;
    }
    
    // 计算移动方向
    FVector MoveDirection;
    if (PathPlanner.IsValid())
    {
        MoveDirection = PathPlanner->CalculateDirection(
            OwnerCharacter->GetWorld(),
            CurrentLocation,
            TargetLocation,
            OwnerCharacter
        );
    }
    else
    {
        MoveDirection = (TargetLocation - CurrentLocation).GetSafeNormal();
        MoveDirection.Z = 0.f;
    }

    // 使用 AddMovementInput 移动
    OwnerCharacter->AddMovementInput(MoveDirection, 1.0f);

    // 更新朝向
    if (!MoveDirection.IsNearlyZero())
    {
        FRotator TargetRotation = MoveDirection.Rotation();
        TargetRotation.Pitch = 0.f;
        TargetRotation.Roll = 0.f;

        FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.f);
        OwnerCharacter->SetActorRotation(NewRotation);
    }
}
