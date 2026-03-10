#pragma once

#include "CoreMinimal.h"
#include "UI/Components/Domain/MASkillListPreviewModels.h"

class FMASkillListPreviewLayout
{
public:
    static FMASkillListPreviewLayoutResult BuildLayout(
        const FMASkillListPreviewModel& Model,
        const FVector2D& CanvasSize,
        const FMASkillListPreviewLayoutConfig& Config);
};
