#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

struct MULTIAGENT_API FMALidarProbeConfig
{
    float MaxDistance = 30000.f;
    float HalfAngleDegrees = 8.f;
    int32 RingCount = 2;
    int32 SamplesPerRing = 6;
    float MinSurfaceNormalAlignment = 0.65f;
    bool bTraceComplex = true;
    bool bIncludeWorldDynamic = false;
};

struct MULTIAGENT_API FMALidarProbeResult
{
    bool bHasHit = false;
    int32 HitCount = 0;
    float MeanDistance = 0.f;
    float MinDistance = 0.f;
    float MaxDistance = 0.f;
    FVector MeanImpactPoint = FVector::ZeroVector;
};

struct MULTIAGENT_API FMALidarProbe
{
    static FMALidarProbeResult SampleCone(
        UWorld& World,
        const FVector& Origin,
        const FVector& Direction,
        const FMALidarProbeConfig& Config,
        const AActor* IgnoredActor = nullptr);
};
