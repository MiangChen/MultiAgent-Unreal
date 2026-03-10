#pragma once

#include "CoreMinimal.h"

enum class EAgentRuntimeFeedbackKind : uint8
{
    None,
    Info,
    Warning,
    Error
};

struct MULTIAGENT_API FAgentRuntimeFeedback
{
    EAgentRuntimeFeedbackKind Kind = EAgentRuntimeFeedbackKind::None;
    FString Message;
    bool bSuccess = true;
};
