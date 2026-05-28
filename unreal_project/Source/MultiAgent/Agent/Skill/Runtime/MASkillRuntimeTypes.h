#pragma once

#include "CoreMinimal.h"
#include "MASkillRuntimeTypes.generated.h"

class AActor;

USTRUCT()
struct FMASkillRuntimeTargets
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> FollowTarget;

    UPROPERTY()
    TWeakObjectPtr<AActor> HazardTargetActor;

    UPROPERTY()
    TWeakObjectPtr<AActor> PhotoTargetActor;

    UPROPERTY()
    TWeakObjectPtr<AActor> BroadcastTargetActor;

    UPROPERTY()
    TWeakObjectPtr<AActor> GuideTargetActor;

    void Reset()
    {
        FollowTarget.Reset();
        HazardTargetActor.Reset();
        PhotoTargetActor.Reset();
        BroadcastTargetActor.Reset();
        GuideTargetActor.Reset();
    }
};

USTRUCT()
struct FMASearchRuntimeResults
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> Object1Actor;

    UPROPERTY()
    FVector Object1Location = FVector::ZeroVector;

    UPROPERTY()
    TWeakObjectPtr<AActor> Object2Actor;

    UPROPERTY()
    FVector Object2Location = FVector::ZeroVector;

    UPROPERTY()
    TArray<FVector> SearchPath;

    void Reset()
    {
        Object1Actor.Reset();
        Object2Actor.Reset();
        Object1Location = FVector::ZeroVector;
        Object2Location = FVector::ZeroVector;
        SearchPath.Empty();
    }
};
