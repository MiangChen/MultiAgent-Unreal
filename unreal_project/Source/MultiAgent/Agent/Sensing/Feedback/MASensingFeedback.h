#pragma once

#include "CoreMinimal.h"
#include "Agent/Sensing/Domain/MASensorTypes.h"

struct MULTIAGENT_API FMASensingOperationFeedback
{
    bool bSuccess = false;
    FString Message;
    FString ResolvedPath;
};

struct MULTIAGENT_API FMASensingActionRequest
{
    bool bSuccess = false;
    EMASensingActionKind Action = EMASensingActionKind::Invalid;
    FString Message;
    FString FilePath;
    int32 Port = 9000;
    float FPS = 30.f;
    int32 Quality = 85;
};

struct MULTIAGENT_API FMASensingStreamFeedback
{
    bool bSuccess = false;
    bool bShouldStartTimer = false;
    bool bShouldCleanup = false;
    float IntervalSeconds = 0.f;
    FString Message;
};
