#pragma once

#include "CoreMinimal.h"
#include "../Infrastructure/MAHUDInputAdapter.h"

class AMAPlayerController;
class UMACameraSensorComponent;

class MULTIAGENT_API FMACameraInputCoordinator
{
public:
    void HandleRightClickPressed(AMAPlayerController* PlayerController) const;
    void HandleRightClickReleased(AMAPlayerController* PlayerController) const;
    void Tick(AMAPlayerController* PlayerController) const;

    void SwitchToNextCamera(AMAPlayerController* PlayerController) const;
    void ReturnToSpectator(AMAPlayerController* PlayerController) const;
    void TakePhoto(AMAPlayerController* PlayerController) const;
    void ToggleTCPStream(AMAPlayerController* PlayerController) const;

private:
    UMACameraSensorComponent* ResolveCurrentCamera(const AMAPlayerController* PlayerController) const;

    FMAHUDInputAdapter HUDInputAdapter;
};
