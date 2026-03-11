#pragma once

#include "CoreMinimal.h"

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
