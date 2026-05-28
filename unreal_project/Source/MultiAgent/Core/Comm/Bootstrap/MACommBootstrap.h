#pragma once

#include "CoreMinimal.h"
#include "Core/Comm/Infrastructure/MACommRuntimeAccess.h"

struct MULTIAGENT_API FCommBootstrap
{
    static bool IsReady(UGameInstance* GameInstance)
    {
        return FCommRuntimeAccess::IsAvailable(GameInstance);
    }
};
