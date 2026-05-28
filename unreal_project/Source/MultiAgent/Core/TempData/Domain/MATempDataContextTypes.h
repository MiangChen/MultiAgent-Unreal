#pragma once

#include "CoreMinimal.h"

struct MULTIAGENT_API FTempDataState
{
    FString ActiveId;
    int32 ItemCount = 0;
    bool bDirty = false;
};
