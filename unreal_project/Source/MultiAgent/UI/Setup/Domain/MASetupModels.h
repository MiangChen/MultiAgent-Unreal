#pragma once

#include "CoreMinimal.h"
#include "MASetupModels.generated.h"

USTRUCT(BlueprintType)
struct FMAAgentSetupConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString AgentType = TEXT("UAV");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayName = TEXT("UAV");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Count = 1;

    FMAAgentSetupConfig() = default;
    FMAAgentSetupConfig(const FString& InType, const FString& InName, int32 InCount = 1)
        : AgentType(InType)
        , DisplayName(InName)
        , Count(InCount)
    {
    }
};

USTRUCT(BlueprintType)
struct FMASetupLaunchRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, int32> AgentConfigs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SelectedScene;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalAgentCount = 0;
};

struct FMASetupWidgetState
{
    TMap<FString, FString> AvailableAgentTypes;
    TArray<FString> AvailableScenes;
    TArray<FMAAgentSetupConfig> AgentConfigs;
    FString SelectedScene = TEXT("CyberCity");
};
