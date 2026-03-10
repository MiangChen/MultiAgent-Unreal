#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Core/TempData/Runtime/MATempDataManager.h"

struct MULTIAGENT_API FTempDataRuntimeAccess
{
    static UMATempDataManager* Resolve(UGameInstance* GameInstance)
    {
        return GameInstance ? GameInstance->GetSubsystem<UMATempDataManager>() : nullptr;
    }

    static bool IsAvailable(UGameInstance* GameInstance)
    {
        return Resolve(GameInstance) != nullptr;
    }
};
