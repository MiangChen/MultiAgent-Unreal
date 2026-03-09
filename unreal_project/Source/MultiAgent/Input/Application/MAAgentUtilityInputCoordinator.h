#pragma once

#include "CoreMinimal.h"

class AMAPlayerController;

class MULTIAGENT_API FMAAgentUtilityInputCoordinator
{
public:
    void HandlePickup() const;
    void HandleDrop() const;
    void HandleSpawnPickupItem() const;
    void HandleSpawnQuadruped() const;
    void HandlePrintAgentInfo(AMAPlayerController* PlayerController) const;
    void HandleDestroyLastAgent(AMAPlayerController* PlayerController) const;
    void HandleJumpSelection(AMAPlayerController* PlayerController) const;
};
