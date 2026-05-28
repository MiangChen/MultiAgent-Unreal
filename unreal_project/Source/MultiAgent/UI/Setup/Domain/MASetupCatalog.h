#pragma once

#include "CoreMinimal.h"
#include "MASetupModels.h"

class FMASetupCatalog
{
public:
    static const TMap<FString, FString>& GetAvailableAgentTypes();
    static const TArray<FString>& GetAvailableScenes();
    static const TMap<FString, FString>& GetSceneToMapPaths();
    static TArray<FMAAgentSetupConfig> BuildDefaultAgentConfigs();
};
