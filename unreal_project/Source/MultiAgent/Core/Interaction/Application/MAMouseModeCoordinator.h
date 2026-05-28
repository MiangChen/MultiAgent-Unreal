#pragma once

#include "CoreMinimal.h"
#include "../Feedback/MAFeedback21.h"
#include "../Domain/MAInputTypes.h"

class AMAPlayerController;

class MULTIAGENT_API FMAMouseModeCoordinator
{
public:
    FMAFeedback21Batch TransitionToMode(AMAPlayerController* PlayerController, EMAMouseMode NewMode) const;
    FString BuildModeStatusText(EMAMouseMode Mode) const;

private:
    void ApplyBaseInputSettings(AMAPlayerController* PlayerController) const;
    void CleanupDeploymentState(AMAPlayerController* PlayerController, EMAMouseMode OldMode, EMAMouseMode NewMode) const;
    bool CanEnterMode(AMAPlayerController* PlayerController, EMAMouseMode NewMode) const;
    FMAFeedback21Batch HandleExit(AMAPlayerController* PlayerController, EMAMouseMode OldMode, EMAMouseMode NewMode) const;
    FMAFeedback21Batch HandleEnter(AMAPlayerController* PlayerController, EMAMouseMode OldMode, EMAMouseMode NewMode) const;
};
