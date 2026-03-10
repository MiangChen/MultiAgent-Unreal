#pragma once

#include "CoreMinimal.h"

enum class ECommFeedbackKind : uint8
{
    None,
    Info,
    Warning,
    Error
};

struct MULTIAGENT_API FCommFeedback
{
    ECommFeedbackKind Kind = ECommFeedbackKind::None;
    FString Message;
    bool bSuccess = true;
};
