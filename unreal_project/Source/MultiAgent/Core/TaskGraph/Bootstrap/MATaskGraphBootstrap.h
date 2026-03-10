#pragma once

#include "CoreMinimal.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"

struct MULTIAGENT_API FTaskGraphBootstrap
{
    static TArray<FMANodeTemplate> BuildDefaultNodeTemplates();
    static FString GetMockResponseExamplePath(const FString& ProjectDir);
};
