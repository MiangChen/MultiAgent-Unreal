#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Core/Editing/Runtime/MAEditModeManager.h"

struct MULTIAGENT_API FEditingRuntimeAccess
{
    static UMAEditModeManager* Resolve(UWorld* World)
    {
        return World ? World->GetSubsystem<UMAEditModeManager>() : nullptr;
    }

    static bool IsAvailable(UWorld* World)
    {
        return Resolve(World) != nullptr;
    }
};
