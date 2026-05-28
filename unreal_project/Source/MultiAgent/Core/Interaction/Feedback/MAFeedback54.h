#pragma once

#include "CoreMinimal.h"

enum class EMAFeedback54Observation : uint8
{
    None,
    CommandDispatch,
    PauseToggle
};

struct FMAFeedback54
{
    EMAFeedback54Observation Observation = EMAFeedback54Observation::None;
    bool bSuccess = false;
    bool bHasObservation = false;
    int32 Count = 0;
    FVector Location = FVector::ZeroVector;
    FString SubjectId;
    FString Message;
};
