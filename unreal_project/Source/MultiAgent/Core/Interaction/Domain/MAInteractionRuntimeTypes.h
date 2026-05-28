#pragma once

#include "CoreMinimal.h"

struct FMAAgentRuntimeStats
{
    int32 Total = 0;
    int32 UAVs = 0;
    int32 Quadrupeds = 0;
    int32 Humanoids = 0;
    TArray<FString> AgentLines;
};

enum class EMACameraStreamEvent : uint8
{
    None,
    Started,
    Stopped
};

struct FMACameraStreamRuntimeResult
{
    EMACameraStreamEvent Event = EMACameraStreamEvent::None;
    FString SensorName;
};
