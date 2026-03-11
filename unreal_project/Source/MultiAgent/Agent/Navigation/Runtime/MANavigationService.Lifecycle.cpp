// MANavigationService.Lifecycle.cpp
// Navigation completion, cleanup, bootstrap, and controller initialization

#include "MANavigationService.h"
#include "Agent/Navigation/Bootstrap/MANavigationBootstrap.h"
#include "../Infrastructure/MAFlightController.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Navigation/PathFollowingComponent.h"

void UMANavigationService::CompleteNavigateSuccess()
{
    SetNavigationState(EMANavigationState::Arrived);
    CompleteNavigation(true, FString::Printf(
        TEXT("Navigate succeeded: Reached destination (%.0f, %.0f, %.0f)"),
        TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
}

void UMANavigationService::CompleteNavigation(bool bSuccess, const FString& Message)
{
    FString OwnerName = OwnerCharacter ? OwnerCharacter->GetName() : TEXT("NULL");
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: CompleteNavigation - Success=%s, Message=%s"),
        *OwnerName, bSuccess ? TEXT("true") : TEXT("false"), *Message);
    
    // 清理
    CleanupNavigation();
    
    // 广播完成事件
    OnNavigationCompleted.Broadcast(bSuccess, Message);
    
    // 延迟重置状态为 Idle（仅在仍处于终态时才重置，避免覆盖新导航的状态）
    ScheduleResetToIdle();
}

void UMANavigationService::CleanupNavigation()
{
    ClearAllNavigationTimers();
    StopAllNavigationDrivers();
    ResetNavigationRuntimeFlags();
    ClearPauseSnapshot();
}

void UMANavigationService::ClearAllNavigationTimers()
{
    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
    if (!World)
    {
        return;
    }

    if (FlightCheckTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(FlightCheckTimerHandle);
        FlightCheckTimerHandle.Invalidate();
    }
    if (ManualNavTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(ManualNavTimerHandle);
        ManualNavTimerHandle.Invalidate();
    }
    if (FollowModeTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(FollowModeTimerHandle);
        FollowModeTimerHandle.Invalidate();
    }
}

void UMANavigationService::StopAllNavigationDrivers()
{
    if (FlightController.IsValid())
    {
        FlightController->CancelFlight();
        FlightController->StopMovement();
    }

    if (!OwnerCharacter)
    {
        return;
    }

    if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
    {
        AICtrl->StopMovement();
        if (UPathFollowingComponent* PathComp = AICtrl->GetPathFollowingComponent())
        {
            PathComp->OnRequestFinished.RemoveAll(this);
        }
    }
}

void UMANavigationService::ResetNavigationRuntimeFlags()
{
    bHasActiveNavMeshRequest = false;
    bUsingManualNavigation = false;
    bIsFollowingActor = false;
    bReturnHomeIsLanding = false;
    bIsReturnHomeActive = false;
    ReturnHomeLandAltitude = 0.f;
    bIsNavigationPaused = false;
    FollowTarget.Reset();
    LastFollowTargetPos = FVector::ZeroVector;
    LastNavMeshNavigateTime = 0.f;
}

void UMANavigationService::InitializeFromConfig()
{
    FMANavigationBootstrap::ApplyConfig(*this, GetWorld());
}

void UMANavigationService::ApplyBootstrapConfig(
    EMAPathPlannerType InPathPlannerType,
    const FMAPathPlannerConfig& InPathPlannerConfig,
    float InMinFlightAltitude,
    float InFollowDistance,
    float InFollowPositionTolerance,
    float InStuckTimeout
)
{
    PathPlannerType = InPathPlannerType;
    PathPlannerConfig = InPathPlannerConfig;
    MinFlightAltitude = InMinFlightAltitude;
    DefaultFollowDistance = InFollowDistance;
    DefaultFollowPositionTolerance = InFollowPositionTolerance;
    StuckTimeout = InStuckTimeout;

    UE_LOG(LogTemp, Verbose, TEXT("[MANavigationService] Bootstrapped config: PlannerType=%d, MinFlightAlt=%.0f, FollowDist=%.0f, FollowTolerance=%.0f, StuckTimeout=%.0f"),
        static_cast<int32>(PathPlannerType),
        MinFlightAltitude,
        DefaultFollowDistance,
        DefaultFollowPositionTolerance,
        StuckTimeout);
}

void UMANavigationService::EnsureFlightControllerInitialized()
{
    if (FlightController.IsValid() || !bIsFlying || !OwnerCharacter)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Initializing FlightController (bIsFixedWing=%s)"),
        *OwnerCharacter->GetName(), bIsFixedWing ? TEXT("true") : TEXT("false"));
    
    if (bIsFixedWing)
    {
        FlightController = MakeUnique<FMAFixedWingFlightController>();
    }
    else
    {
        FlightController = MakeUnique<FMAMultiRotorFlightController>();
    }
    FlightController->Initialize(OwnerCharacter);
}
