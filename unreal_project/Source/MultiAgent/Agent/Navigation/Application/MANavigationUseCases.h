#pragma once

#include "CoreMinimal.h"
#include "Agent/Navigation/Feedback/MANavigationFeedback.h"

class ACharacter;
struct FPathFollowingResult;

struct MULTIAGENT_API FMANavigationUseCases
{
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

    static bool ShouldRefreshGroundFollowNav(
        bool bHasLastFollowTargetPos,
        float FollowPosChange2D,
        float CurrentTimeSeconds,
        float LastNavMeshNavigateTime,
        float NavMeshFollowUpdateInterval);
};
