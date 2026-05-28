#pragma once

#include "CoreMinimal.h"
#include "UI/Components/Domain/MATaskGraphPreviewModels.h"

class FMATaskGraphPreviewLayout
{
public:
    static FMATaskGraphPreviewLayoutResult BuildLayout(
        const FMATaskGraphPreviewModel& Model,
        const FVector2D& CanvasSize,
        const FMATaskGraphPreviewLayoutConfig& Config);
};
