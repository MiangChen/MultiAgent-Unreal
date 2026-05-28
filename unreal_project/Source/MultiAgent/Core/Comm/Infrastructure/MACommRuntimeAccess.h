#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Core/Comm/Runtime/MACommSubsystem.h"

struct MULTIAGENT_API FCommRuntimeAccess
{
    static UMACommSubsystem* Resolve(UGameInstance* GameInstance)
    {
        return GameInstance ? GameInstance->GetSubsystem<UMACommSubsystem>() : nullptr;
    }

    static bool IsAvailable(UGameInstance* GameInstance)
    {
        return Resolve(GameInstance) != nullptr;
    }
};
