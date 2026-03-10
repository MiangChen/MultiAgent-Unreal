#pragma once

#include "CoreMinimal.h"
#include "Core/Camera/Infrastructure/MACameraRuntimeAccess.h"

struct MULTIAGENT_API FCameraBootstrap
{
    static bool IsReady(UWorld* World)
    {
        return FCameraRuntimeAccess::IsAvailable(World);
    }
};
