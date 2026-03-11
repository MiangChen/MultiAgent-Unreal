#pragma once

#include "CoreMinimal.h"
#include "Agent/Navigation/Domain/MANavigationTypes.h"

#include "Navigation/PathFollowingComponent.h"

enum class EMANavigationFeedbackSeverity : uint8
{
    Info,
    Warning,
    Error,
};

struct MULTIAGENT_API FMANavigationCommandFeedback
{
    bool bSuccess = false;
    EMANavigationFeedbackSeverity Severity = EMANavigationFeedbackSeverity::Info;
    FString Message;
};

struct MULTIAGENT_API FMANavigationRequestLifecycleFeedback
{
    bool bCanStart = false;
    bool bShouldCleanupPrevious = false;
    bool bShouldConfigureFlightController = false;
    EMANavigationState NextState = EMANavigationState::Idle;
    FString FailureMessage;
};

struct MULTIAGENT_API FMANavigationRequestStateFeedback
{
    FVector TargetLocation = FVector::ZeroVector;
    FVector StartLocation = FVector::ZeroVector;
    float AcceptanceRadius = 0.f;
    bool bUsingManualNavigation = false;
    bool bHasActiveNavMeshRequest = false;
    float ManualNavStuckTime = 0.f;
    bool bIsReturnHomeActive = false;
};

struct MULTIAGENT_API FMANavigationCancelFeedback
{
    bool bShouldCancel = false;
    EMANavigationState NextState = EMANavigationState::Cancelled;
    FString CompletionMessage;
};

enum class EMANavigationGroundRequestAction : uint8
{
    ContinueNavMesh,
    FallbackToManual,
    CompleteSuccess,
};

struct MULTIAGENT_API FMANavigationGroundRequestFeedback
{
    EMANavigationGroundRequestAction Action = EMANavigationGroundRequestAction::ContinueNavMesh;
    bool bShouldScheduleFalseSuccessCheck = false;
    FString CompletionMessage;
};

enum class EMANavigationCompletionAction : uint8
{
    CompleteSuccess,
    CompleteFailure,
    FallbackToManual,
};

struct MULTIAGENT_API FMANavigationCompletionFeedback
{
    EMANavigationCompletionAction Action = EMANavigationCompletionAction::CompleteFailure;
    bool bWasFalseSuccess = false;
    FString CompletionMessage;
};

enum class EMANavigationManualUpdateAction : uint8
{
    Continue,
    CompleteSuccess,
    CompleteFailure,
};

struct MULTIAGENT_API FMANavigationManualUpdateFeedback
{
    EMANavigationManualUpdateAction Action = EMANavigationManualUpdateAction::Continue;
    float NextStuckTime = 0.f;
    bool bShouldUpdateLastLocation = false;
    FString CompletionMessage;
};

enum class EMANavigationFollowUpdateAction : uint8
{
    Continue,
    RotateOnly,
    HandleLostTarget,
};

struct MULTIAGENT_API FMANavigationFollowUpdateFeedback
{
    EMANavigationFollowUpdateAction Action = EMANavigationFollowUpdateAction::Continue;
    FString FailureMessage;
};

struct MULTIAGENT_API FMANavigationFollowLifecycleFeedback
{
    bool bCanStart = false;
    bool bShouldStopPreviousFollow = false;
    bool bShouldCancelActiveNavigation = false;
    EMANavigationState NextState = EMANavigationState::Idle;
    FString FailureMessage;
};

enum class EMANavigationFlightOperationAction : uint8
{
    Continue,
    CompleteSuccess,
    CompleteFailure,
};

struct MULTIAGENT_API FMANavigationFlightOperationFeedback
{
    EMANavigationFlightOperationAction Action = EMANavigationFlightOperationAction::Continue;
    EMANavigationState NextState = EMANavigationState::Idle;
    FString CompletionMessage;
};

enum class EMANavigationReturnHomeAction : uint8
{
    Continue,
    StartLanding,
    CompleteSuccess,
    CompleteFailure,
};

struct MULTIAGENT_API FMANavigationReturnHomeFeedback
{
    EMANavigationReturnHomeAction Action = EMANavigationReturnHomeAction::Continue;
    EMANavigationState NextState = EMANavigationState::Idle;
    FString CompletionMessage;
    FString LogMessage;
};
