#pragma once

#include "CoreMinimal.h"
#include "../Domain/MASetupModels.h"

class AHUD;
class APlayerController;
class UUserWidget;

class FMASetupRuntimeAdapter
{
public:
    bool ShouldShowSetupUI(const AHUD* HUD) const;
    void ScheduleNextTick(AHUD* HUD, TFunction<void()> Callback) const;
    void ApplySetupInputMode(APlayerController* PlayerController, UUserWidget* FocusWidget) const;
    bool SaveLaunchRequest(const AHUD* HUD, const FMASetupLaunchRequest& Request) const;
    bool OpenSelectedScene(const AHUD* HUD, const FString& SelectedScene) const;
};
