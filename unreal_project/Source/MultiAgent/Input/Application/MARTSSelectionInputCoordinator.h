#pragma once

#include "CoreMinimal.h"
#include "../Infrastructure/MAHUDInputAdapter.h"

class AMAPlayerController;

class MULTIAGENT_API FMARTSSelectionInputCoordinator
{
public:
    void HandleLeftClickStarted(AMAPlayerController* PlayerController) const;
    void HandleLeftClickReleased(AMAPlayerController* PlayerController) const;
    void HandleMiddleClickNavigate(AMAPlayerController* PlayerController) const;
    void Tick(AMAPlayerController* PlayerController) const;

private:
    bool IsCtrlPressed(const AMAPlayerController* PlayerController) const;

    FMAHUDInputAdapter HUDInputAdapter;
};
