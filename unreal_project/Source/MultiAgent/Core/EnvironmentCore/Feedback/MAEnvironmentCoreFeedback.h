#pragma once

#include "CoreMinimal.h"

enum class EEnvironmentCoreFeedbackKind : uint8
{
    None,
    Info,
    Warning,
    Error
};

struct MULTIAGENT_API FEnvironmentCoreFeedback
{
    EEnvironmentCoreFeedbackKind Kind = EEnvironmentCoreFeedbackKind::None;
    FString Message;
    bool bSuccess = true;
};
