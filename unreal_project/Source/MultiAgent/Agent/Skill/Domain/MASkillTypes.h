#pragma once

#include "CoreMinimal.h"
#include "MASkillTypes.generated.h"

UENUM(BlueprintType)
enum class ESearchMode : uint8
{
    Coverage UMETA(DisplayName = "Coverage Search"),
    Patrol UMETA(DisplayName = "Patrol")
};

USTRUCT(BlueprintType)
struct FMASemanticTarget
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Class;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FString> Features;

    bool IsEmpty() const { return Class.IsEmpty() && Type.IsEmpty(); }
    bool IsGround() const { return Class.Equals(TEXT("ground"), ESearchCase::IgnoreCase); }
    bool IsRobot() const { return Class.Equals(TEXT("robot"), ESearchCase::IgnoreCase) || Type.Equals(TEXT("UGV"), ESearchCase::IgnoreCase); }
};

USTRUCT(BlueprintType)
struct FMASkillParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AcceptanceRadius = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FollowDistance = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> SearchBoundary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMASemanticTarget SearchTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SearchScanWidth = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ESearchMode SearchMode = ESearchMode::Coverage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PatrolWaitTime = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PatrolCycleLimit = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMASemanticTarget PlaceObject1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMASemanticTarget PlaceObject2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TakeOffHeight = 2500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LandHeight = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector HomeLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector ChargingStationLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ChargingStationId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bChargingStationFound = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMASemanticTarget CommonTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CommonTargetObjectId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DefaultSearchRadius = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PhotoFOVOverride = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString BroadcastMessage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMASemanticTarget GuideTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString GuideTargetObjectId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector GuideDestination = FVector::ZeroVector;

};
