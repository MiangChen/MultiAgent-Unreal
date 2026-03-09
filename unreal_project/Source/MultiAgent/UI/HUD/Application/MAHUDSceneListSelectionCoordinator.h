#pragma once

#include "CoreMinimal.h"

class AMAHUD;

class MULTIAGENT_API FMAHUDSceneListSelectionCoordinator
{
public:
    void HandleGoalClicked(AMAHUD* HUD, const FString& GoalId) const;
    void HandleZoneClicked(AMAHUD* HUD, const FString& ZoneId) const;
};
