#pragma once

#include "CoreMinimal.h"
#include "Core/EnvironmentCore/Infrastructure/MAEnvironmentCoreRuntimeAccess.h"

struct MULTIAGENT_API FEnvironmentCoreBootstrap
{
    static bool IsReady(UWorld* World)
    {
        return FEnvironmentCoreRuntimeAccess::IsAvailable(World);
    }
};
