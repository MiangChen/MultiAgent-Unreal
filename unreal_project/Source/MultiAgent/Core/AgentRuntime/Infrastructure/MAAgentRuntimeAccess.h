#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Core/AgentRuntime/Runtime/MAAgentManager.h"

struct MULTIAGENT_API FAgentRuntimeRuntimeAccess
{
    static UMAAgentManager* Resolve(UWorld* World)
    {
        return World ? World->GetSubsystem<UMAAgentManager>() : nullptr;
    }

    static bool IsAvailable(UWorld* World)
    {
        return Resolve(World) != nullptr;
    }
};
