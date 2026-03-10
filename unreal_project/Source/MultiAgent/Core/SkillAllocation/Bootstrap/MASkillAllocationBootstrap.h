#pragma once

#include "CoreMinimal.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"

struct MULTIAGENT_API FMASkillAllocationBootstrap
{
    static FMASkillAllocationData BuildEmptyData(
        const FString& Name = TEXT("Skill Allocation"),
        const FString& Description = TEXT(""));
};
