#pragma once

#include "CoreMinimal.h"

class AMACharacter;
class AActor;

namespace MAObservationSkillRuntime
{
bool IsAircraft(const AMACharacter& Character);
float ResolveMinFlightAltitude(const AMACharacter& Character, float DefaultMinFlightAltitude);
FVector CalculateStandOffPosition(
    const AMACharacter& Character,
    const FVector& TargetLocation,
    float StandOffDistance,
    bool bIsAircraft,
    float MinFlightAltitude);

struct FTurnTowardTargetResult
{
    bool bReachedTargetYaw = false;
    float YawDiff = 0.f;
};

FTurnTowardTargetResult StepTurnTowardTarget(
    AActor& Actor,
    const FVector& TargetLocation,
    float DeltaTime,
    float InterpSpeed = 5.f,
    float CompletionYawTolerance = 5.f);
}
