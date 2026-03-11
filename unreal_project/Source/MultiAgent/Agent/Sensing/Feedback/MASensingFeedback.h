#pragma once

#include "CoreMinimal.h"

struct MULTIAGENT_API FMASensingOperationFeedback
{
    bool bSuccess = false;
    FString Message;
    FString ResolvedPath;
};
