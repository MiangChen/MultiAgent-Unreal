#pragma once

#include "CoreMinimal.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"

struct FMATaskPlannerActionResult
{
    bool bSuccess = false;
    bool bGraphChanged = false;
    bool bShouldPersist = false;
    bool bHasData = false;
    FString Message;
    TArray<FString> DetailLines;
    FMATaskGraphData Data;
};
