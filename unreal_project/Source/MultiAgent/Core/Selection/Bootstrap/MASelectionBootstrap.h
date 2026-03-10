#pragma once

#include "CoreMinimal.h"
#include "Core/Selection/Infrastructure/MASelectionRuntimeAccess.h"

struct MULTIAGENT_API FSelectionBootstrap
{
    static bool IsReady(UWorld* World)
    {
        return FSelectionRuntimeAccess::IsAvailable(World);
    }
};
