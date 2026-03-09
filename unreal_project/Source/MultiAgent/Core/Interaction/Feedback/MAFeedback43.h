#pragma once

#include "CoreMinimal.h"

enum class EMAFeedback43Event : uint8
{
    None,
    ExecutionAccepted,
    ExecutionRejected
};

struct FMAFeedback43
{
    EMAFeedback43Event Event = EMAFeedback43Event::None;
    bool bSuccess = false;
    bool bIsWarning = false;
    int32 Count = 0;
    FString SubjectId;
    FString Message;
};
