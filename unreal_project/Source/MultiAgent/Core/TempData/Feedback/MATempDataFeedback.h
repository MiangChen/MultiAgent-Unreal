#pragma once

#include "CoreMinimal.h"

enum class ETempDataFeedbackKind : uint8
{
    None,
    Info,
    Warning,
    Error
};

struct MULTIAGENT_API FTempDataFeedback
{
    ETempDataFeedbackKind Kind = ETempDataFeedbackKind::None;
    FString Message;
    bool bSuccess = true;
};
