#pragma once

#include "CoreMinimal.h"
#include "../../Mode/Application/MAEditWidgetStateCoordinator.h"
#include "../../Mode/Application/MAModifyWidgetStateCoordinator.h"

class AMAHUD;

class MULTIAGENT_API FMAHUDModeWidgetLifecycleCoordinator
{
public:
    void ShowModifyWidget(AMAHUD* HUD) const;
    void HideModifyWidget(AMAHUD* HUD) const;
    void ShowEditWidget(AMAHUD* HUD) const;
    void HideEditWidget(AMAHUD* HUD) const;

private:
    FMAEditWidgetStateCoordinator EditWidgetStateCoordinator;
    FMAModifyWidgetStateCoordinator ModifyWidgetStateCoordinator;
};
