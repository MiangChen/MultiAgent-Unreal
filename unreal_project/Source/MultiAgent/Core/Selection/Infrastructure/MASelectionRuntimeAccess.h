#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Core/Selection/Runtime/MASelectionManager.h"

struct MULTIAGENT_API FSelectionRuntimeAccess
{
    static UMASelectionManager* Resolve(UWorld* World)
    {
        return World ? World->GetSubsystem<UMASelectionManager>() : nullptr;
    }

    static bool IsAvailable(UWorld* World)
    {
        return Resolve(World) != nullptr;
    }
};
