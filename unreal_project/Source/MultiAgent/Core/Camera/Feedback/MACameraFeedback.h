#pragma once

#include "CoreMinimal.h"

enum class ECameraFeedbackKind : uint8
{
    None,
    Info,
    Warning,
    Error
};

struct MULTIAGENT_API FCameraFeedback
{
    ECameraFeedbackKind Kind = ECameraFeedbackKind::None;
    FString Message;
    bool bSuccess = true;
};
