#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Core/Camera/Runtime/MAViewportManager.h"

struct MULTIAGENT_API FCameraRuntimeAccess
{
    static UMAViewportManager* Resolve(UWorld* World)
    {
        return World ? World->GetSubsystem<UMAViewportManager>() : nullptr;
    }

    static bool IsAvailable(UWorld* World)
    {
        return Resolve(World) != nullptr;
    }
};
