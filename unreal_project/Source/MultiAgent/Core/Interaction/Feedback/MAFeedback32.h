#pragma once

#include "CoreMinimal.h"

enum class EMAFeedback32Event : uint8
{
    None,
    ModeTransitionAccepted,
    ModeTransitionRejected,
    DeploymentQueueChanged,
    SelectionChanged,
    SceneActionPrepared
};

struct FMAFeedback32
{
    EMAFeedback32Event Event = EMAFeedback32Event::None;
    bool bSuccess = false;
    int32 Count = 0;
    FString SubjectId;
    FString Message;
};
