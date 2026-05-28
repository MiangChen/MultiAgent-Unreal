// MAHUDDelegateCoordinator.h
// HUD delegate binding coordination.

#pragma once

#include "CoreMinimal.h"

class AMAHUD;

class MULTIAGENT_API FMAHUDDelegateCoordinator
{
public:
    void BindWidgetDelegates(AMAHUD* HUD) const;
    void BindControllerEvents(AMAHUD* HUD) const;
};
