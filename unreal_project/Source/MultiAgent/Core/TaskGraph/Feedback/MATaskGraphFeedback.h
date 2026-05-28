#pragma once

#include "CoreMinimal.h"

enum class ETaskGraphFeedbackKind : uint8
{
    None,
    Info,
    Success,
    Warning,
    Error
};

struct MULTIAGENT_API FTaskGraphFeedback
{
    ETaskGraphFeedbackKind Kind = ETaskGraphFeedbackKind::None;
    FString Message;
    bool bSuccess = true;
};
