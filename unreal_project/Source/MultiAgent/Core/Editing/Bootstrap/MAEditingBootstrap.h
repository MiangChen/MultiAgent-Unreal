#pragma once

#include "CoreMinimal.h"
#include "Core/Editing/Infrastructure/MAEditingRuntimeAccess.h"

struct MULTIAGENT_API FEditingBootstrap
{
    static bool IsReady(UWorld* World)
    {
        return FEditingRuntimeAccess::IsAvailable(World);
    }
};
