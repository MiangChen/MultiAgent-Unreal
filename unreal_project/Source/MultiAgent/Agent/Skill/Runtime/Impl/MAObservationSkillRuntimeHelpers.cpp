#include "MAObservationSkillRuntimeHelpers.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/CharacterRuntime/Runtime/MAUAVCharacter.h"
#include "GameFramework/Actor.h"

namespace MAObservationSkillRuntime
{
bool IsAircraft(const AMACharacter& Character)
{
    return Character.AgentType == EMAAgentType::UAV ||
           Character.AgentType == EMAAgentType::FixedWingUAV;
}

float ResolveMinFlightAltitude(const AMACharacter& Character, const float DefaultMinFlightAltitude)
{
    if (const AMAUAVCharacter* UAV = Cast<AMAUAVCharacter>(&Character))
    {
        return UAV->MinFlightAltitude;
    }

    return DefaultMinFlightAltitude;
}

FVector CalculateStandOffPosition(
    const AMACharacter& Character,
    const FVector& TargetLocation,
    const float StandOffDistance,
    const bool bIsAircraft,
    const float MinFlightAltitude)
{
    const FVector RobotLocation = Character.GetActorLocation();

    if (bIsAircraft)
    {
        FVector HorizontalDirection = TargetLocation - RobotLocation;
        HorizontalDirection.Z = 0.f;
        HorizontalDirection = HorizontalDirection.GetSafeNormal();

        FVector StandOffPosition = TargetLocation - HorizontalDirection * StandOffDistance;
        StandOffPosition.Z = FMath::Max(RobotLocation.Z, MinFlightAltitude);
        return StandOffPosition;
    }

    const FVector Direction = (TargetLocation - RobotLocation).GetSafeNormal();
    return TargetLocation - Direction * StandOffDistance;
}

FTurnTowardTargetResult StepTurnTowardTarget(
    AActor& Actor,
    const FVector& TargetLocation,
    const float DeltaTime,
    const float InterpSpeed,
    const float CompletionYawTolerance)
{
    FTurnTowardTargetResult Result;

    FVector DirectionToTarget = TargetLocation - Actor.GetActorLocation();
    DirectionToTarget.Z = 0.f;
    DirectionToTarget = DirectionToTarget.GetSafeNormal();

    if (DirectionToTarget.IsNearlyZero())
    {
        Result.bReachedTargetYaw = true;
        return Result;
    }

    const FRotator CurrentRotation = Actor.GetActorRotation();
    FRotator TargetRotation = DirectionToTarget.Rotation();
    TargetRotation.Pitch = 0.f;
    TargetRotation.Roll = 0.f;

    const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, InterpSpeed);
    Actor.SetActorRotation(NewRotation);

    Result.YawDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(NewRotation.Yaw, TargetRotation.Yaw));
    Result.bReachedTargetYaw = Result.YawDiff < CompletionYawTolerance;
    return Result;
}
}
