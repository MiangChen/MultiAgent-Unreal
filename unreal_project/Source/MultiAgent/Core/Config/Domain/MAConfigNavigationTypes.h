#pragma once

#include "CoreMinimal.h"
#include "MAConfigNavigationTypes.generated.h"

USTRUCT(BlueprintType)
struct FMAFlightConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float MinAltitude = 800.f;

    UPROPERTY(BlueprintReadOnly)
    float DefaultAltitude = 1000.f;

    UPROPERTY(BlueprintReadOnly)
    float MaxSpeed = 600.f;

    UPROPERTY(BlueprintReadOnly)
    float ObstacleDetectionRange = 800.f;

    UPROPERTY(BlueprintReadOnly)
    float ObstacleAvoidanceRadius = 150.f;

    UPROPERTY(BlueprintReadOnly)
    float AcceptanceRadius = 200.f;
};

USTRUCT(BlueprintType)
struct FMAGroundNavigationConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float AcceptanceRadius = 200.f;

    UPROPERTY(BlueprintReadOnly)
    float StuckTimeout = 10.f;
};

USTRUCT(BlueprintType)
struct FMAFollowConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float Distance = 300.f;

    UPROPERTY(BlueprintReadOnly)
    float PositionTolerance = 200.f;

    UPROPERTY(BlueprintReadOnly)
    float ContinuousTimeThreshold = 30.f;
};

USTRUCT(BlueprintType)
struct FMAGuideConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString TargetMoveMode = TEXT("navmesh");

    UPROPERTY(BlueprintReadOnly)
    float WaitDistanceThreshold = 500.f;
};

USTRUCT(BlueprintType)
struct FMAHandleHazardConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float SafeDistance = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float Duration = 15.0f;

    UPROPERTY(BlueprintReadOnly)
    float SpraySpeed = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float SprayWidth = 30.f;
};

USTRUCT(BlueprintType)
struct FMATakePhotoConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float PhotoDistance = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float PhotoDuration = 3.0f;

    UPROPERTY(BlueprintReadOnly)
    float CameraFOV = 60.f;

    UPROPERTY(BlueprintReadOnly)
    float CameraForwardOffset = 50.f;
};

USTRUCT(BlueprintType)
struct FMABroadcastConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float BroadcastDistance = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float BroadcastDuration = 5.0f;

    UPROPERTY(BlueprintReadOnly)
    float EffectSpeed = 500.f;

    UPROPERTY(BlueprintReadOnly)
    float EffectWidth = 30.f;

    UPROPERTY(BlueprintReadOnly)
    float ShockRate = 5.0f;
};
