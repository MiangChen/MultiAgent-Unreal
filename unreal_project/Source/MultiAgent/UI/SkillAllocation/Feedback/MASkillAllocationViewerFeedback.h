#pragma once

#include "CoreMinimal.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"

struct FMASkillAllocationViewerActionResult
{
    bool bSuccess = false;
    bool bDataChanged = false;
    bool bShouldPersist = false;
    bool bHasData = false;
    FString Message;
    TArray<FString> DetailLines;
    FMASkillAllocationData Data;
};
