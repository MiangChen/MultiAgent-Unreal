#pragma once

#include "CoreMinimal.h"
#include "Core/AgentRuntime/Infrastructure/MAAgentRuntimeRuntimeAccess.h"

struct MULTIAGENT_API FAgentRuntimeBootstrap
{
    static bool IsReady(UWorld* World)
    {
        return FAgentRuntimeRuntimeAccess::IsAvailable(World);
    }
};
