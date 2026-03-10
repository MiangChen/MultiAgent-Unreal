#pragma once

#include "CoreMinimal.h"

enum class ESelectionFeedbackKind : uint8
{
    None,
    Info,
    Warning,
    Error
};

struct MULTIAGENT_API FSelectionFeedback
{
    ESelectionFeedbackKind Kind = ESelectionFeedbackKind::None;
    FString Message;
    bool bSuccess = true;
};
