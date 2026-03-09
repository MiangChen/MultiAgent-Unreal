#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAInteractionRuntimeTypes.h"
#include "../Feedback/MAFeedback21.h"

class AMAPlayerController;

class MULTIAGENT_API FMACameraInputCoordinator
{
public:
    void HandleRightClickPressed(AMAPlayerController* PlayerController) const;
    void HandleRightClickReleased(AMAPlayerController* PlayerController) const;
    void Tick(AMAPlayerController* PlayerController) const;

    void SwitchToNextCamera(AMAPlayerController* PlayerController) const;
    void ReturnToSpectator(AMAPlayerController* PlayerController) const;
    FMAFeedback21Batch TakePhoto(AMAPlayerController* PlayerController) const;
    FMAFeedback21Batch ToggleTCPStream(AMAPlayerController* PlayerController) const;
};
