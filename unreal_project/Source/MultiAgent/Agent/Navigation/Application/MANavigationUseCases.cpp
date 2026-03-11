#include "Agent/Navigation/Application/MANavigationUseCases.h"

#include "GameFramework/Character.h"

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
