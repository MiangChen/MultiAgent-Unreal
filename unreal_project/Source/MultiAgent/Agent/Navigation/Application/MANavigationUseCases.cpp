#include "Agent/Navigation/Application/MANavigationUseCases.h"

#include "GameFramework/Character.h"

FMANavigationRequestLifecycleFeedback FMANavigationUseCases::BuildNavigateRequestLifecycle(
    const bool bHasOwnerCharacter,
    const bool bHasActiveNavigationOperation,
    const bool bIsFlying)
{
    FMANavigationRequestLifecycleFeedback Feedback;
    if (!bHasOwnerCharacter)
    {
        Feedback.FailureMessage = TEXT("NavigateTo failed: No owner character");
        return Feedback;
    }

    Feedback.bCanStart = true;
    Feedback.bShouldCleanupPrevious = bHasActiveNavigationOperation;
    Feedback.bShouldConfigureFlightController = bIsFlying;
    Feedback.NextState = EMANavigationState::Navigating;
    return Feedback;
}

FMANavigationCancelFeedback FMANavigationUseCases::BuildCancelNavigationFeedback(
    const bool bHasActiveNavigationOperation)
{
    FMANavigationCancelFeedback Feedback;
    Feedback.bShouldCancel = bHasActiveNavigationOperation;
    Feedback.CompletionMessage = TEXT("Navigation cancelled");
    return Feedback;
}

bool FMANavigationUseCases::IsActiveNavigationState(const EMANavigationState State)
{
    return State == EMANavigationState::Navigating ||
           State == EMANavigationState::TakingOff ||
           State == EMANavigationState::Landing;
}

bool FMANavigationUseCases::IsTerminalNavigationState(const EMANavigationState State)
{
    return State == EMANavigationState::Arrived ||
           State == EMANavigationState::Failed ||
           State == EMANavigationState::Cancelled;
}

bool FMANavigationUseCases::ShouldResetToIdle(const EMANavigationState State)
{
    return IsTerminalNavigationState(State);
}

FMANavigationRequestStateFeedback FMANavigationUseCases::BuildNavigateRequestState(
    const FVector Destination,
    const float AcceptanceRadius,
    const FVector StartLocation)
{
    FMANavigationRequestStateFeedback Feedback;
    Feedback.TargetLocation = Destination;
    Feedback.AcceptanceRadius = AcceptanceRadius;
    Feedback.StartLocation = StartLocation;
    return Feedback;
}

float FMANavigationUseCases::BuildNavigationProgress(
    const EMANavigationState CurrentState,
    const bool bHasOwnerCharacter,
    const float StartDistance,
    const float CurrentDistance)
{
    if (CurrentState != EMANavigationState::Navigating || !bHasOwnerCharacter)
    {
        return CurrentState == EMANavigationState::Arrived ? 1.0f : 0.0f;
    }

    if (StartDistance < KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }

    return FMath::Clamp(1.0f - (CurrentDistance / StartDistance), 0.0f, 1.0f);
}

FMANavigationCommandFeedback FMANavigationUseCases::BuildFlightCommandPrecheck(
    const ACharacter* OwnerCharacter,
    const bool bIsFlying,
    const bool bHasFlightController,
    const TCHAR* CommandName)
{
    FMANavigationCommandFeedback Feedback;

    if (!OwnerCharacter)
    {
        Feedback.Severity = EMANavigationFeedbackSeverity::Error;
        Feedback.Message = FString::Printf(TEXT("%s failed: No owner character"), CommandName);
        return Feedback;
    }

    if (!bIsFlying)
    {
        Feedback.Severity = EMANavigationFeedbackSeverity::Warning;
        Feedback.Message = FString::Printf(TEXT("%s failed: Not a flying vehicle"), CommandName);
        return Feedback;
    }

    if (!bHasFlightController)
    {
        Feedback.Severity = EMANavigationFeedbackSeverity::Warning;
        Feedback.Message = FString::Printf(TEXT("%s failed: FlightController not initialized"), CommandName);
        return Feedback;
    }

    Feedback.bSuccess = true;
    Feedback.Severity = EMANavigationFeedbackSeverity::Info;
    Feedback.Message = FString::Printf(TEXT("%s precheck passed"), CommandName);
    return Feedback;
}

FMANavigationGroundRequestFeedback FMANavigationUseCases::BuildGroundMoveRequestDecision(
    EPathFollowingRequestResult::Type Result,
    const float DistanceToTarget,
    const float ActualDistance,
    const float AcceptanceRadius,
    const FVector& TargetLocation)
{
    FMANavigationGroundRequestFeedback Feedback;

    if (Result == EPathFollowingRequestResult::Failed)
    {
        Feedback.Action = EMANavigationGroundRequestAction::FallbackToManual;
        return Feedback;
    }

    if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        if (ActualDistance < AcceptanceRadius * 2.f)
        {
            Feedback.Action = EMANavigationGroundRequestAction::CompleteSuccess;
            Feedback.CompletionMessage = FString::Printf(
                TEXT("Navigate succeeded: Already at destination (%.0f, %.0f, %.0f)"),
                TargetLocation.X,
                TargetLocation.Y,
                TargetLocation.Z);
            return Feedback;
        }

        Feedback.Action = EMANavigationGroundRequestAction::FallbackToManual;
        return Feedback;
    }

    Feedback.Action = EMANavigationGroundRequestAction::ContinueNavMesh;
    Feedback.bShouldScheduleFalseSuccessCheck =
        Result == EPathFollowingRequestResult::RequestSuccessful &&
        DistanceToTarget > AcceptanceRadius * 3.f;
    return Feedback;
}

FMANavigationCompletionFeedback FMANavigationUseCases::BuildGroundCompletionDecision(
    const FPathFollowingResult& Result,
    const float ActualDistance,
    const float AcceptanceRadius,
    const FVector& TargetLocation)
{
    FMANavigationCompletionFeedback Feedback;

    const bool bTooFar = ActualDistance > AcceptanceRadius * 2.f;
    if (bTooFar && (Result.IsSuccess() ||
        Result.Code == EPathFollowingResult::Blocked ||
        Result.Code == EPathFollowingResult::OffPath))
    {
        Feedback.Action = EMANavigationCompletionAction::FallbackToManual;
        Feedback.bWasFalseSuccess = Result.IsSuccess();
        return Feedback;
    }

    if (Result.IsSuccess())
    {
        Feedback.Action = EMANavigationCompletionAction::CompleteSuccess;
        Feedback.CompletionMessage = FString::Printf(
            TEXT("Navigate succeeded: Reached destination (%.0f, %.0f, %.0f)"),
            TargetLocation.X,
            TargetLocation.Y,
            TargetLocation.Z);
        return Feedback;
    }

    Feedback.Action = EMANavigationCompletionAction::CompleteFailure;
    switch (Result.Code)
    {
        case EPathFollowingResult::Blocked:
            Feedback.CompletionMessage = TEXT("Navigate failed: Path blocked by obstacle");
            break;
        case EPathFollowingResult::OffPath:
            Feedback.CompletionMessage = TEXT("Navigate failed: Lost navigation path");
            break;
        case EPathFollowingResult::Aborted:
            Feedback.CompletionMessage = TEXT("Navigate failed: Navigation aborted");
            break;
        default:
            Feedback.CompletionMessage = TEXT("Navigate failed: Unknown error");
            break;
    }
    return Feedback;
}

FMANavigationManualUpdateFeedback FMANavigationUseCases::BuildManualNavigationUpdate(
    const float DistanceToTarget2D,
    const float MovedDistance,
    const float DeltaTime,
    const float CurrentStuckTime,
    const float AcceptanceRadius,
    const float StuckTimeout,
    const FVector& TargetLocation)
{
    FMANavigationManualUpdateFeedback Feedback;

    if (DistanceToTarget2D < AcceptanceRadius)
    {
        Feedback.Action = EMANavigationManualUpdateAction::CompleteSuccess;
        Feedback.CompletionMessage = FString::Printf(
            TEXT("Navigate succeeded: Reached destination (%.0f, %.0f, %.0f)"),
            TargetLocation.X,
            TargetLocation.Y,
            TargetLocation.Z);
        return Feedback;
    }

    if (MovedDistance < 50.f)
    {
        Feedback.NextStuckTime = CurrentStuckTime + DeltaTime;
        if (Feedback.NextStuckTime > StuckTimeout)
        {
            Feedback.Action = EMANavigationManualUpdateAction::CompleteFailure;
            Feedback.CompletionMessage = TEXT("Navigate failed: Path blocked, unable to reach destination");
            return Feedback;
        }

        return Feedback;
    }

    Feedback.Action = EMANavigationManualUpdateAction::Continue;
    Feedback.NextStuckTime = 0.f;
    Feedback.bShouldUpdateLastLocation = true;
    return Feedback;
}

FMANavigationFollowUpdateFeedback FMANavigationUseCases::BuildFlightFollowUpdate(
    const bool bIsFollowingActor,
    const bool bHasOwnerCharacter,
    const bool bHasFollowTarget,
    const float DistanceToFollowPos,
    const float AcceptanceRadius)
{
    FMANavigationFollowUpdateFeedback Feedback;

    if (!bIsFollowingActor || !bHasOwnerCharacter || !bHasFollowTarget)
    {
        Feedback.Action = EMANavigationFollowUpdateAction::HandleLostTarget;
        Feedback.FailureMessage = TEXT("Follow failed: Target lost");
        return Feedback;
    }

    if (DistanceToFollowPos < AcceptanceRadius)
    {
        Feedback.Action = EMANavigationFollowUpdateAction::RotateOnly;
        return Feedback;
    }

    Feedback.Action = EMANavigationFollowUpdateAction::Continue;
    return Feedback;
}

FMANavigationFollowLifecycleFeedback FMANavigationUseCases::BuildFollowStartLifecycle(
    const bool bHasOwnerCharacter,
    const bool bHasTarget,
    const bool bIsAlreadyFollowing,
    const EMANavigationState CurrentState)
{
    FMANavigationFollowLifecycleFeedback Feedback;
    if (!bHasOwnerCharacter || !bHasTarget)
    {
        Feedback.FailureMessage = TEXT("FollowActor failed: Invalid owner or target");
        return Feedback;
    }

    Feedback.bCanStart = true;
    Feedback.bShouldStopPreviousFollow = bIsAlreadyFollowing;
    Feedback.bShouldCancelActiveNavigation = IsActiveNavigationState(CurrentState);
    Feedback.NextState = EMANavigationState::Navigating;
    return Feedback;
}

FMANavigationFlightOperationFeedback FMANavigationUseCases::BuildFlightOperationUpdate(
    const EMANavigationState CurrentState,
    const bool bHasOwnerCharacter,
    const bool bHasFlightController,
    const bool bHasReachedFlightTerminalState,
    const float CurrentAltitudeMeters)
{
    FMANavigationFlightOperationFeedback Feedback;
    if (!bHasOwnerCharacter || !bHasFlightController)
    {
        Feedback.Action = EMANavigationFlightOperationAction::CompleteFailure;
        Feedback.CompletionMessage = CurrentState == EMANavigationState::TakingOff
            ? TEXT("TakeOff failed: Character or FlightController lost")
            : TEXT("Land failed: Character or FlightController lost");
        return Feedback;
    }

    if (!bHasReachedFlightTerminalState)
    {
        return Feedback;
    }

    Feedback.Action = EMANavigationFlightOperationAction::CompleteSuccess;
    Feedback.NextState = EMANavigationState::Arrived;
    Feedback.CompletionMessage = CurrentState == EMANavigationState::TakingOff
        ? FString::Printf(TEXT("TakeOff succeeded: Reached altitude %.0fm"), CurrentAltitudeMeters)
        : FString::Printf(TEXT("Land succeeded: Landed at altitude %.0fm"), CurrentAltitudeMeters);
    return Feedback;
}

FMANavigationReturnHomeFeedback FMANavigationUseCases::BuildReturnHomeUpdate(
    const bool bHasOwnerCharacter,
    const bool bHasFlightController,
    const bool bIsLandingPhase,
    const bool bHasArrivedAtHome,
    const bool bHasFinishedLanding,
    const FVector& CurrentLocation)
{
    FMANavigationReturnHomeFeedback Feedback;
    if (!bHasOwnerCharacter || !bHasFlightController)
    {
        Feedback.Action = EMANavigationReturnHomeAction::CompleteFailure;
        Feedback.CompletionMessage = TEXT("ReturnHome failed: Character or FlightController lost");
        return Feedback;
    }

    if (bIsLandingPhase)
    {
        if (!bHasFinishedLanding)
        {
            return Feedback;
        }

        Feedback.Action = EMANavigationReturnHomeAction::CompleteSuccess;
        Feedback.NextState = EMANavigationState::Arrived;
        Feedback.CompletionMessage = FString::Printf(
            TEXT("ReturnHome succeeded: Landed at (%.0f, %.0f, %.0f)"),
            CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z);
        return Feedback;
    }

    if (!bHasArrivedAtHome)
    {
        return Feedback;
    }

    Feedback.Action = EMANavigationReturnHomeAction::StartLanding;
    Feedback.NextState = EMANavigationState::Landing;
    Feedback.LogMessage = TEXT("ReturnHome - Arrived at home, starting landing");
    return Feedback;
}

bool FMANavigationUseCases::ShouldRefreshGroundFollowNav(
    const bool bHasLastFollowTargetPos,
    const float FollowPosChange2D,
    const float CurrentTimeSeconds,
    const float LastNavMeshNavigateTime,
    const float NavMeshFollowUpdateInterval)
{
    constexpr float FollowPosChangeThreshold = 100.f;
    if (!bHasLastFollowTargetPos)
    {
        return true;
    }

    if (FollowPosChange2D > FollowPosChangeThreshold)
    {
        return true;
    }

    return (CurrentTimeSeconds - LastNavMeshNavigateTime) >= NavMeshFollowUpdateInterval;
}
