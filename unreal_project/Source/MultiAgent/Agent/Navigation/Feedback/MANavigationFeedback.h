#pragma once

#include "CoreMinimal.h"

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
