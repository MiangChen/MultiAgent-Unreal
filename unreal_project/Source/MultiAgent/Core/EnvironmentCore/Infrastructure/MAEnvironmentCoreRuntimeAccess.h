#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Core/EnvironmentCore/Runtime/MAEnvironmentManager.h"

struct MULTIAGENT_API FEnvironmentCoreRuntimeAccess
{
    static UMAEnvironmentManager* Resolve(UWorld* World)
    {
        return World ? World->GetSubsystem<UMAEnvironmentManager>() : nullptr;
    }

    static bool IsAvailable(UWorld* World)
    {
        return Resolve(World) != nullptr;
    }
};
