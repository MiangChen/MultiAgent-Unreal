#pragma once

#include "CoreMinimal.h"
#include "../Domain/MASelectionHUDModel.h"

class AMASelectionHUD;

class MULTIAGENT_API FMASelectionHUDCoordinator
{
public:
    FMASelectionHUDFrameModel BuildFrameModel(const AMASelectionHUD* HUD, bool bShowAgentCircles) const;
};
