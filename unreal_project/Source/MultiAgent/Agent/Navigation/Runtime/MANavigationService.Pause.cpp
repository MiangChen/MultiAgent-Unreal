// MANavigationService.Pause.cpp
// Navigation pause/resume snapshot handling

#include "MANavigationService.h"
#include "../Infrastructure/MAFlightController.h"
#include "AIController.h"
#include "GameFramework/Character.h"

//=========================================================================
// 暂停/恢复实现
//=========================================================================

void UMANavigationService::PauseNavigation()
{
    if (bIsNavigationPaused || CurrentState == EMANavigationState::Idle)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s: PauseNavigation SKIPPED (bIsNavigationPaused=%s, CurrentState=%d, bIsFlying=%s, FlightTimer=%s)"),
            OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"),
            bIsNavigationPaused ? TEXT("true") : TEXT("false"),
            (int32)CurrentState,
            bIsFlying ? TEXT("true") : TEXT("false"),
            FlightCheckTimerHandle.IsValid() ? TEXT("valid") : TEXT("invalid"));
        return;
    }

    CapturePauseSnapshot();
    PauseActiveDrivers();

    bIsNavigationPaused = true;

    const bool bPausedFollowing = PauseSnapshot.Mode == EMANavigationPauseMode::Following;
    const bool bPausedManual = PauseSnapshot.Mode == EMANavigationPauseMode::Manual;
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Navigation PAUSED (flying=%s, following=%s, manualNav=%s)"),
        OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"),
        bIsFlying ? TEXT("true") : TEXT("false"),
        bPausedFollowing ? TEXT("true") : TEXT("false"),
        bPausedManual ? TEXT("true") : TEXT("false"));
}

void UMANavigationService::ResumeNavigation()
{
    if (!bIsNavigationPaused)
    {
        return;
    }

    bIsNavigationPaused = false;

    ResumeFromPauseSnapshot();
    ClearPauseSnapshot();

    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: Navigation RESUMED"),
        OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"));
}
EMANavigationPauseMode UMANavigationService::ResolvePauseMode() const
{
    if (bIsFollowingActor)
    {
        return EMANavigationPauseMode::Following;
    }
    if (bIsFlying)
    {
        return EMANavigationPauseMode::Flying;
    }
    if (bUsingManualNavigation)
    {
        return EMANavigationPauseMode::Manual;
    }
    return EMANavigationPauseMode::Ground;
}

void UMANavigationService::CapturePauseSnapshot()
{
    PauseSnapshot.TargetLocation = TargetLocation;
    PauseSnapshot.AcceptanceRadius = AcceptanceRadius;
    PauseSnapshot.Mode = ResolvePauseMode();
    PauseSnapshot.State = CurrentState;
    PauseSnapshot.FollowTarget = FollowTarget;
    PauseSnapshot.FollowDistance = FollowDistance;
    PauseSnapshot.bReturnHomeIsLanding = bReturnHomeIsLanding;
    PauseSnapshot.bReturnHomeActive = bIsReturnHomeActive;
    PauseSnapshot.ReturnHomeLandAltitude = ReturnHomeLandAltitude;
    PauseSnapshot.bIsValid = true;
}

void UMANavigationService::ClearPauseSnapshot()
{
    PauseSnapshot.Reset();
}

void UMANavigationService::PauseActiveDrivers()
{
    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;

    // 停止飞行控制器（悬停）
    if (bIsFlying && FlightController.IsValid())
    {
        FlightController->StopMovement();
    }

    // 停止地面 NavMesh 路径跟随
    // 注意：必须先设 bHasActiveNavMeshRequest = false，再调用 StopMovement()
    // 因为 StopMovement() 会同步触发 OnNavMeshMoveCompleted 回调
    if (!bIsFlying && bHasActiveNavMeshRequest && OwnerCharacter)
    {
        bHasActiveNavMeshRequest = false;
        if (AAIController* AICtrl = Cast<AAIController>(OwnerCharacter->GetController()))
        {
            AICtrl->StopMovement();
        }
    }

    if (World && FollowModeTimerHandle.IsValid())
    {
        World->GetTimerManager().PauseTimer(FollowModeTimerHandle);
    }
    if (World && ManualNavTimerHandle.IsValid())
    {
        World->GetTimerManager().PauseTimer(ManualNavTimerHandle);
    }
    if (World && FlightCheckTimerHandle.IsValid())
    {
        World->GetTimerManager().PauseTimer(FlightCheckTimerHandle);
    }
}

void UMANavigationService::ResumeFromPauseSnapshot()
{
    if (!PauseSnapshot.bIsValid)
    {
        return;
    }

    TargetLocation = PauseSnapshot.TargetLocation;
    AcceptanceRadius = PauseSnapshot.AcceptanceRadius;
    FollowDistance = PauseSnapshot.FollowDistance;
    bReturnHomeIsLanding = PauseSnapshot.bReturnHomeIsLanding;
    bIsReturnHomeActive = PauseSnapshot.bReturnHomeActive;
    ReturnHomeLandAltitude = PauseSnapshot.ReturnHomeLandAltitude;

    UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
    switch (PauseSnapshot.Mode)
    {
        case EMANavigationPauseMode::Following:
            if (World && FollowModeTimerHandle.IsValid())
            {
                World->GetTimerManager().UnPauseTimer(FollowModeTimerHandle);
            }
            else if (PauseSnapshot.FollowTarget.IsValid())
            {
                FollowActor(PauseSnapshot.FollowTarget.Get(), FollowDistance, AcceptanceRadius);
            }
            break;
        case EMANavigationPauseMode::Flying:
            ResumePausedFlyingOperation(World);
            break;
        case EMANavigationPauseMode::Manual:
            if (World && ManualNavTimerHandle.IsValid())
            {
                World->GetTimerManager().UnPauseTimer(ManualNavTimerHandle);
            }
            else
            {
                StartManualNavigation();
            }
            break;
        case EMANavigationPauseMode::Ground:
            // 恢复地面 NavMesh 导航：重新发起导航请求
            NavigateTo(TargetLocation, AcceptanceRadius);
            break;
        case EMANavigationPauseMode::None:
        default:
            break;
    }
}

void UMANavigationService::ResumePausedFlyingOperation(UWorld* World)
{
    EnsureFlightControllerInitialized();
    if (FlightController.IsValid())
    {
        FlightController->SetAcceptanceRadius(AcceptanceRadius);

        if (PauseSnapshot.bReturnHomeActive)
        {
            if (PauseSnapshot.bReturnHomeIsLanding || PauseSnapshot.State == EMANavigationState::Landing)
            {
                FlightController->Land(TargetLocation);
            }
            else
            {
                FlightController->FlyTo(TargetLocation);
            }
        }
        else if (PauseSnapshot.State == EMANavigationState::TakingOff)
        {
            FlightController->TakeOff(TargetLocation.Z);
        }
        else if (PauseSnapshot.State == EMANavigationState::Landing)
        {
            FlightController->Land(TargetLocation);
        }
        else
        {
            FlightController->FlyTo(TargetLocation);
        }
    }

    if (World && FlightCheckTimerHandle.IsValid())
    {
        World->GetTimerManager().UnPauseTimer(FlightCheckTimerHandle);
        return;
    }

    if (PauseSnapshot.bReturnHomeActive)
    {
        StartFlightTimer(&UMANavigationService::UpdateReturnHome);
    }
    else if (PauseSnapshot.State == EMANavigationState::TakingOff || PauseSnapshot.State == EMANavigationState::Landing)
    {
        StartFlightTimer(&UMANavigationService::UpdateFlightOperation);
    }
    else
    {
        StartFlightTimer(&UMANavigationService::UpdateFlightNavigation);
    }
}
