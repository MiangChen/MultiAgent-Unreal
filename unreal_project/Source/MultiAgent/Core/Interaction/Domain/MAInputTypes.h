#pragma once

#include "CoreMinimal.h"
#include "MAInputTypes.generated.h"

UENUM(BlueprintType)
enum class EMAMouseMode : uint8
{
    Select UMETA(DisplayName = "Select"),
    Deployment UMETA(DisplayName = "Deployment"),
    Modify UMETA(DisplayName = "Modify"),
    Edit UMETA(DisplayName = "Edit")
};

USTRUCT(BlueprintType)
struct FMAPendingDeployment
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString AgentType;

    UPROPERTY(BlueprintReadWrite)
    int32 Count = 0;

    FMAPendingDeployment() = default;
    FMAPendingDeployment(const FString& InType, int32 InCount)
        : AgentType(InType)
        , Count(InCount)
    {
    }
};
