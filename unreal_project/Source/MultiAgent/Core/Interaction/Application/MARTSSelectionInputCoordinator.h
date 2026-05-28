#pragma once

#include "CoreMinimal.h"
#include "../Feedback/MAFeedback21.h"

class AMAPlayerController;

class MULTIAGENT_API FMARTSSelectionInputCoordinator
{
public:
    void HandleLeftClickStarted(AMAPlayerController* PlayerController) const;
    void HandleLeftClickReleased(AMAPlayerController* PlayerController) const;
    FMAFeedback21Batch HandleMiddleClickNavigate(AMAPlayerController* PlayerController) const;
    FMAFeedback21Batch Tick(AMAPlayerController* PlayerController) const;

private:
    bool IsCtrlPressed(const AMAPlayerController* PlayerController) const;
};
