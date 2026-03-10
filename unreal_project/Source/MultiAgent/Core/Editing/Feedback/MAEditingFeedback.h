#pragma once

#include "CoreMinimal.h"

enum class EEditingFeedbackKind : uint8
{
    None,
    Info,
    Warning,
    Error
};

struct MULTIAGENT_API FEditingFeedback
{
    EEditingFeedbackKind Kind = EEditingFeedbackKind::None;
    FString Message;
    bool bSuccess = true;
};
