#pragma once

#include "CoreMinimal.h"
#include "Agent/Navigation/Feedback/MANavigationFeedback.h"

class ACharacter;
struct FPathFollowingResult;

struct MULTIAGENT_API FMANavigationUseCases
{
    static FMANavigationRequestLifecycleFeedback BuildNavigateRequestLifecycle(
        bool bHasOwnerCharacter,
        bool bHasActiveNavigationOperation,
        bool bIsFlying);

    static FMANavigationCancelFeedback BuildCancelNavigationFeedback(
        bool bHasActiveNavigationOperation);

    static bool IsActiveNavigationState(EMANavigationState State);
    static bool IsTerminalNavigationState(EMANavigationState State);
    static bool ShouldResetToIdle(EMANavigationState State);

    static FMANavigationRequestStateFeedback BuildNavigateRequestState(
        FVector Destination,
        float AcceptanceRadius,
        FVector StartLocation);

    static float BuildNavigationProgress(
        EMANavigationState CurrentState,
        bool bHasOwnerCharacter,
        float StartDistance,
        float CurrentDistance);

    static FMANavigationCommandFeedback BuildFlightCommandPrecheck(
        const ACharacter* OwnerCharacter,
        bool bIsFlying,
        bool bHasFlightController,
        const TCHAR* CommandName);

    static FMANavigationGroundRequestFeedback BuildGroundMoveRequestDecision(
        EPathFollowingRequestResult::Type Result,
        float DistanceToTarget,
        float ActualDistance,
        float AcceptanceRadius,
        const FVector& TargetLocation);

    static FMANavigationCompletionFeedback BuildGroundCompletionDecision(
        const FPathFollowingResult& Result,
        float ActualDistance,
        float AcceptanceRadius,
        const FVector& TargetLocation);

    static FMANavigationManualUpdateFeedback BuildManualNavigationUpdate(
        float DistanceToTarget2D,
        float MovedDistance,
        float DeltaTime,
        float CurrentStuckTime,
        float AcceptanceRadius,
        float StuckTimeout,
        const FVector& TargetLocation);

    static FMANavigationFollowUpdateFeedback BuildFlightFollowUpdate(
        bool bIsFollowingActor,
        bool bHasOwnerCharacter,
        bool bHasFollowTarget,
        float DistanceToFollowPos,
        float AcceptanceRadius);

    static FMANavigationFollowLifecycleFeedback BuildFollowStartLifecycle(
        bool bHasOwnerCharacter,
        bool bHasTarget,
        bool bIsAlreadyFollowing,
        EMANavigationState CurrentState);

    static FMANavigationFlightOperationFeedback BuildFlightOperationUpdate(
        EMANavigationState CurrentState,
        bool bHasOwnerCharacter,
        bool bHasFlightController,
        bool bHasReachedFlightTerminalState,
        float CurrentAltitudeMeters);

    static FMANavigationReturnHomeFeedback BuildReturnHomeUpdate(
        bool bHasOwnerCharacter,
        bool bHasFlightController,
        bool bIsLandingPhase,
        bool bHasArrivedAtHome,
        bool bHasFinishedLanding,
        const FVector& CurrentLocation);

    static bool ShouldRefreshGroundFollowNav(
        bool bHasLastFollowTargetPos,
        float FollowPosChange2D,
        float CurrentTimeSeconds,
        float LastNavMeshNavigateTime,
        float NavMeshFollowUpdateInterval);
};
