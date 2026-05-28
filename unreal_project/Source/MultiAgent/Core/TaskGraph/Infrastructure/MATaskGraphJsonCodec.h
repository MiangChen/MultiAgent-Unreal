#pragma once

#include "CoreMinimal.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"

struct MULTIAGENT_API FTaskGraphJsonCodec
{
    static FString Serialize(const FMATaskGraphData& Data);
    static bool TryParseJson(const FString& Json, FMATaskGraphData& OutData, FString& OutError);
    static bool TryParseResponseJson(const FString& Json, FMATaskGraphData& OutData, FString& OutError);
};
