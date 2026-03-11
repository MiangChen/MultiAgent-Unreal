#pragma once

#include "CoreMinimal.h"

namespace MAPathPlannerInternal
{
inline const TArray<float>& GetRaycastHeights()
{
    static const TArray<float> Values = {30.f, 60.f, 90.f};
    return Values;
}

inline constexpr float CliffCheckHeight = 100.f;
inline constexpr float CliffCheckDepth = 300.f;
inline constexpr float MaxAllowedHeightDiff = 150.f;
inline constexpr float ElevationRayStartOffset = 1000.f;
inline constexpr float ElevationRayEndOffset = 1000.f;
inline constexpr float ObstacleCheckHeight = 60.f;
}
