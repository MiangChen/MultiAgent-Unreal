#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Core/Command/Runtime/MACommandManager.h"

struct MULTIAGENT_API FCommandRuntimeAccess
{
    static UMACommandManager* Resolve(UWorld* World)
    {
        return World ? World->GetSubsystem<UMACommandManager>() : nullptr;
    }

    static bool IsAvailable(UWorld* World)
    {
        return Resolve(World) != nullptr;
    }
};
