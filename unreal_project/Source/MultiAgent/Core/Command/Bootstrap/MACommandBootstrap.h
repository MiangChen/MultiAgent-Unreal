#pragma once

#include "CoreMinimal.h"
#include "Core/Command/Infrastructure/MACommandRuntimeAccess.h"

struct MULTIAGENT_API FCommandBootstrap
{
    static bool IsReady(UWorld* World)
    {
        return FCommandRuntimeAccess::IsAvailable(World);
    }
};
