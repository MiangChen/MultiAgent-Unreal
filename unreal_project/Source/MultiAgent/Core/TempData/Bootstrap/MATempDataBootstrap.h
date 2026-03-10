#pragma once

#include "CoreMinimal.h"
#include "Core/TempData/Infrastructure/MATempDataRuntimeAccess.h"

struct MULTIAGENT_API FTempDataBootstrap
{
    static bool IsReady(UGameInstance* GameInstance)
    {
        return FTempDataRuntimeAccess::IsAvailable(GameInstance);
    }
};
