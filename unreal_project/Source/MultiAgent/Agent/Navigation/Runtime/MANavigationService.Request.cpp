// MANavigationService.Request.cpp
// 导航请求/取消/进度与请求态初始化

#include "MANavigationService.h"
#include "Agent/Navigation/Application/MANavigationUseCases.h"
#include "GameFramework/Character.h"

bool UMANavigationService::NavigateTo(FVector Destination, float InAcceptanceRadius, bool bSmoothArrival)
{
    const FMANavigationRequestLifecycleFeedback Feedback =
        FMANavigationUseCases::BuildNavigateRequestLifecycle(
            OwnerCharacter != nullptr,
            FMANavigationUseCases::IsActiveNavigationState(CurrentState),
            bIsFlying);
    if (!Feedback.bCanStart)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MANavigationService] %s"), *Feedback.FailureMessage);
        return false;
    }

    if (Feedback.bShouldCleanupPrevious)
    {
        UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: NavigateTo - Cancelling previous operation (state=%d)"),
            *OwnerCharacter->GetName(), (int32)CurrentState);
        CleanupNavigation();
    }

    InitializeNavigateRequest(Destination, InAcceptanceRadius);

    if (Feedback.bShouldConfigureFlightController)
    {
        EnsureFlightControllerInitialized();
        if (FlightController.IsValid())
        {
            FlightController->SetSmoothArrival(bSmoothArrival);
        }
    }

    SetNavigationState(Feedback.NextState);

    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: NavigateTo (%.0f, %.0f, %.0f), AcceptanceRadius=%.0f, bIsFlying=%s, bSmoothArrival=%s"),
        *OwnerCharacter->GetName(), Destination.X, Destination.Y, Destination.Z, AcceptanceRadius,
        bIsFlying ? TEXT("true") : TEXT("false"),
        bSmoothArrival ? TEXT("true") : TEXT("false"));

    return bIsFlying ? StartFlightNavigation() : StartGroundNavigation();
}

void UMANavigationService::CancelNavigation()
{
    const FMANavigationCancelFeedback Feedback =
        FMANavigationUseCases::BuildCancelNavigationFeedback(
            FMANavigationUseCases::IsActiveNavigationState(CurrentState));
    if (!Feedback.bShouldCancel)
    {
        return;
    }

    const FString OwnerName = OwnerCharacter ? OwnerCharacter->GetName() : TEXT("NULL");
    UE_LOG(LogTemp, Log, TEXT("[MANavigationService] %s: CancelNavigation (CurrentState=%d)"),
        *OwnerName, (int32)CurrentState);

    CleanupNavigation();
    SetNavigationState(Feedback.NextState);
    CompleteNavigation(false, Feedback.CompletionMessage);
}

bool UMANavigationService::IsNavigating() const
{
    return FMANavigationUseCases::IsActiveNavigationState(CurrentState);
}

float UMANavigationService::GetNavigationProgress() const
{
    const float StartDistance = FVector::Dist(StartLocation, TargetLocation);
    const float CurrentDistance = OwnerCharacter
        ? FVector::Dist(OwnerCharacter->GetActorLocation(), TargetLocation)
        : 0.0f;
    return FMANavigationUseCases::BuildNavigationProgress(
        CurrentState,
        OwnerCharacter != nullptr,
        StartDistance,
        CurrentDistance);
}

bool UMANavigationService::HasActiveNavigationOperation() const
{
    return FMANavigationUseCases::IsActiveNavigationState(CurrentState);
}

bool UMANavigationService::IsNavigationTerminalState() const
{
    return FMANavigationUseCases::IsTerminalNavigationState(CurrentState);
}

void UMANavigationService::InitializeNavigateRequest(const FVector& Destination, const float InAcceptanceRadius)
{
    const FMANavigationRequestStateFeedback Feedback =
        FMANavigationUseCases::BuildNavigateRequestState(
            Destination,
            InAcceptanceRadius,
            OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector);
    TargetLocation = Feedback.TargetLocation;
    AcceptanceRadius = Feedback.AcceptanceRadius;
    StartLocation = Feedback.StartLocation;
    bUsingManualNavigation = Feedback.bUsingManualNavigation;
    bHasActiveNavMeshRequest = Feedback.bHasActiveNavMeshRequest;
    ManualNavStuckTime = Feedback.ManualNavStuckTime;
    bIsReturnHomeActive = Feedback.bIsReturnHomeActive;
}

void UMANavigationService::ScheduleResetToIdle()
{
    if (!OwnerCharacter)
    {
        return;
    }

    UWorld* World = OwnerCharacter->GetWorld();
    if (!World)
    {
        return;
    }

    FTimerHandle ResetHandle;
    World->GetTimerManager().SetTimer(
        ResetHandle,
        [this]()
        {
            if (FMANavigationUseCases::ShouldResetToIdle(CurrentState))
            {
                SetNavigationState(EMANavigationState::Idle);
            }
        },
        0.1f,
        false
    );
}
