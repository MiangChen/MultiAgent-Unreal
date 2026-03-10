#pragma once

#include "CoreMinimal.h"
#include "MAConfigEntityTypes.generated.h"

USTRUCT(BlueprintType)
struct FMAAgentConfigData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString ID;

    UPROPERTY(BlueprintReadOnly)
    FString Label;

    UPROPERTY(BlueprintReadOnly)
    FString Type;

    UPROPERTY(BlueprintReadOnly)
    FVector Position = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadOnly)
    bool bAutoPosition = true;

    UPROPERTY(BlueprintReadOnly)
    float BatteryLevel = 100.f;
};

USTRUCT(BlueprintType)
struct FMAPatrolConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bEnabled = false;

    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> Waypoints;

    UPROPERTY(BlueprintReadOnly)
    bool bLoop = true;

    UPROPERTY(BlueprintReadOnly)
    float WaitTime = 2.0f;
};

USTRUCT(BlueprintType)
struct FMAEnvironmentObjectConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString ID;

    UPROPERTY(BlueprintReadOnly)
    FString Label;

    UPROPERTY(BlueprintReadOnly)
    FString Type;

    UPROPERTY(BlueprintReadOnly)
    FVector Position = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(BlueprintReadOnly)
    TMap<FString, FString> Features;

    UPROPERTY(BlueprintReadOnly)
    FMAPatrolConfig Patrol;
};
