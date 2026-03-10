#pragma once

#include "CoreMinimal.h"
#include "Core/Comm/Domain/MACommSkillTypes.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"

struct MULTIAGENT_API FMASkillAllocationParseFeedback
{
    bool bSuccess = false;
    bool bWarning = false;
    FString Message;
    FMASkillAllocationData Data;
};

struct MULTIAGENT_API FMASkillAllocationSkillListBuildFeedback
{
    bool bSuccess = false;
    FString Message;
    FMASkillListMessage SkillList;
};
