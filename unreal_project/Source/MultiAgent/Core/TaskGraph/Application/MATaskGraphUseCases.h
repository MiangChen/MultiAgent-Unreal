#pragma once

#include "CoreMinimal.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"
#include "Core/TaskGraph/Feedback/MATaskGraphFeedback.h"

struct MULTIAGENT_API FTaskGraphLoadResult
{
    bool bSuccess = false;
    FMATaskGraphData Data;
    FTaskGraphFeedback Feedback;
};

struct MULTIAGENT_API FTaskGraphUseCases
{
    static TArray<FMANodeTemplate> BuildDefaultNodeTemplates();
    static FTaskGraphLoadResult ParseJson(const FString& JsonString);
    static FTaskGraphLoadResult ParseResponseJson(const FString& JsonString);
    static FTaskGraphLoadResult ParseFlexibleJson(const FString& JsonString);
    static FTaskGraphLoadResult LoadResponseExample(const FString& ProjectDir);
    static FTaskGraphLoadResult LoadResponseExampleFile(const FString& FilePath);
    static FString Serialize(const FMATaskGraphData& Data);
};
