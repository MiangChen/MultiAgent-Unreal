#include "Agent/Navigation/Infrastructure/MALidarProbe.h"

#include "Engine/World.h"

namespace
{
FVector BuildPerpendicularAxis(const FVector& Direction)
{
    const FVector Reference = FMath::Abs(Direction.Z) < 0.99f ? FVector::UpVector : FVector::RightVector;
    return FVector::CrossProduct(Direction, Reference).GetSafeNormal();
}

bool TraceSingleSample(
    UWorld& World,
    const FVector& Origin,
    const FVector& SampleDirection,
    const FMALidarProbeConfig& Config,
    const AActor* IgnoredActor,
    FHitResult& OutHit)
{
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MALidarProbe), Config.bTraceComplex);
    if (IgnoredActor)
    {
        QueryParams.AddIgnoredActor(IgnoredActor);
    }

    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    if (Config.bIncludeWorldDynamic)
    {
        ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    }

    const FVector End = Origin + SampleDirection * Config.MaxDistance;
    if (!World.LineTraceSingleByObjectType(OutHit, Origin, End, ObjectParams, QueryParams))
    {
        return false;
    }

    const float SurfaceAlignment = FVector::DotProduct(OutHit.ImpactNormal.GetSafeNormal(), -SampleDirection);
    return SurfaceAlignment >= Config.MinSurfaceNormalAlignment;
}
}

FMALidarProbeResult FMALidarProbe::SampleCone(
    UWorld& World,
    const FVector& Origin,
    const FVector& Direction,
    const FMALidarProbeConfig& Config,
    const AActor* IgnoredActor)
{
    FMALidarProbeResult Result;

    const FVector Forward = Direction.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        return Result;
    }

    const FVector Right = BuildPerpendicularAxis(Forward);
    const FVector Up = FVector::CrossProduct(Right, Forward).GetSafeNormal();

    TArray<FVector> SampleDirections;
    SampleDirections.Reserve(1 + Config.RingCount * Config.SamplesPerRing * FMath::Max(1, Config.RingCount));
    SampleDirections.Add(Forward);

    for (int32 RingIndex = 1; RingIndex <= Config.RingCount; ++RingIndex)
    {
        const float RingAlpha = static_cast<float>(RingIndex) / static_cast<float>(Config.RingCount);
        const float RingAngleRadians = FMath::DegreesToRadians(Config.HalfAngleDegrees * RingAlpha);
        const float RingCos = FMath::Cos(RingAngleRadians);
        const float RingSin = FMath::Sin(RingAngleRadians);
        const int32 RingSamples = FMath::Max(1, Config.SamplesPerRing * RingIndex);

        for (int32 SampleIndex = 0; SampleIndex < RingSamples; ++SampleIndex)
        {
            const float AzimuthRadians = (2.f * PI * static_cast<float>(SampleIndex)) / static_cast<float>(RingSamples);
            const FVector Lateral = Right * FMath::Cos(AzimuthRadians) + Up * FMath::Sin(AzimuthRadians);
            const FVector SampleDirection = (Forward * RingCos + Lateral * RingSin).GetSafeNormal();
            SampleDirections.Add(SampleDirection);
        }
    }

    FVector AccumulatedImpactPoint = FVector::ZeroVector;
    float AccumulatedDistance = 0.f;
    float MinDistance = TNumericLimits<float>::Max();
    float MaxDistance = 0.f;

    for (const FVector& SampleDirection : SampleDirections)
    {
        FHitResult Hit;
        if (!TraceSingleSample(World, Origin, SampleDirection, Config, IgnoredActor, Hit))
        {
            continue;
        }

        Result.HitCount++;
        AccumulatedImpactPoint += Hit.ImpactPoint;
        AccumulatedDistance += Hit.Distance;
        MinDistance = FMath::Min(MinDistance, Hit.Distance);
        MaxDistance = FMath::Max(MaxDistance, Hit.Distance);
    }

    if (Result.HitCount <= 0)
    {
        return Result;
    }

    Result.bHasHit = true;
    Result.MeanImpactPoint = AccumulatedImpactPoint / static_cast<float>(Result.HitCount);
    Result.MeanDistance = AccumulatedDistance / static_cast<float>(Result.HitCount);
    Result.MinDistance = MinDistance;
    Result.MaxDistance = MaxDistance;
    return Result;
}
