#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAInputTypes.h"

class AMAPlayerController;

class MULTIAGENT_API FMAMouseModeCoordinator
{
public:
    bool TransitionToMode(AMAPlayerController* PlayerController, EMAMouseMode NewMode) const;
    FString BuildModeStatusText(EMAMouseMode Mode) const;

private:
    void ApplyBaseInputSettings(AMAPlayerController* PlayerController) const;
    void CleanupDeploymentState(AMAPlayerController* PlayerController, EMAMouseMode OldMode, EMAMouseMode NewMode) const;
    bool CanEnterMode(AMAPlayerController* PlayerController, EMAMouseMode NewMode) const;
    void HandleExit(AMAPlayerController* PlayerController, EMAMouseMode OldMode, EMAMouseMode NewMode) const;
    void HandleEnter(AMAPlayerController* PlayerController, EMAMouseMode OldMode, EMAMouseMode NewMode) const;
};
