#pragma once

#include "CoreMinimal.h"

enum class ECommandFeedbackKind : uint8
{
    None,
    Info,
    Warning,
    Error
};

struct MULTIAGENT_API FCommandFeedback
{
    ECommandFeedbackKind Kind = ECommandFeedbackKind::None;
    FString Message;
    bool bSuccess = true;
};
