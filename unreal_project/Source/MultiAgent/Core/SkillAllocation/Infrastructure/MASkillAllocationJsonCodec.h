#pragma once

#include "CoreMinimal.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"

struct MULTIAGENT_API FMASkillAllocationJsonCodec
{
    static bool Deserialize(const FString& JsonString, FMASkillAllocationData& OutData, FString& OutError);
    static FString Serialize(const FMASkillAllocationData& Data);
};
