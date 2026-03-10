#pragma once

#include "CoreMinimal.h"
#include "../Domain/MASetupModels.h"

class AMASetupHUD;
class UUserWidget;

class FMASetupHUDCoordinator
{
public:
    bool ShouldShowSetupUI(const AMASetupHUD& HUD) const;
    void ScheduleWidgetCreation(AMASetupHUD& HUD, TFunction<void()> Callback) const;
    void ApplyWidgetInputMode(AMASetupHUD& HUD, UUserWidget* Widget) const;
    bool StartSimulation(AMASetupHUD& HUD, const FMASetupLaunchRequest& Request) const;
};
