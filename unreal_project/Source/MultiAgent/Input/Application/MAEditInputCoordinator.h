#pragma once

#include "CoreMinimal.h"
#include "../Infrastructure/MAHUDInputAdapter.h"

class AMAPlayerController;

class MULTIAGENT_API FMAEditInputCoordinator
{
public:
    void HandleLeftClick(AMAPlayerController* PlayerController) const;
    void EnterMode(AMAPlayerController* PlayerController) const;
    void ExitMode(AMAPlayerController* PlayerController) const;

private:
    FMAHUDInputAdapter HUDInputAdapter;
};
