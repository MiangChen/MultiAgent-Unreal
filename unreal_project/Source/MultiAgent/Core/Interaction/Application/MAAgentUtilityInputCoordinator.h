#pragma once

#include "CoreMinimal.h"
#include "../Feedback/MAFeedback21.h"

class AMAPlayerController;

class MULTIAGENT_API FMAAgentUtilityInputCoordinator
{
public:
    void HandlePickup() const;
    void HandleDrop() const;
    FMAFeedback21Batch HandleSpawnPickupItem() const;
    FMAFeedback21Batch HandleSpawnQuadruped() const;
    FMAFeedback21Batch HandlePrintAgentInfo(AMAPlayerController* PlayerController) const;
    FMAFeedback21Batch HandleDestroyLastAgent(AMAPlayerController* PlayerController) const;
    void HandleJumpSelection(AMAPlayerController* PlayerController) const;
};
