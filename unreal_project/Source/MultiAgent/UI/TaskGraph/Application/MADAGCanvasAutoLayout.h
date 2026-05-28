#pragma once

#include "CoreMinimal.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"

class FMADAGCanvasAutoLayout
{
public:
    static TMap<FString, FVector2D> BuildTopologicalLayout(
        const FMATaskGraphData& Data,
        float StartX = 80.0f,
        float StartY = 50.0f,
        float NodeSpacingX = 280.0f,
        float NodeSpacingY = 180.0f);
};
