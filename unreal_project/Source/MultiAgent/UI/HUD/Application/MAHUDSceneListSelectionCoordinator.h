#pragma once

#include "CoreMinimal.h"
#include "../Infrastructure/MAHUDEditRuntimeAdapter.h"

class AMAHUD;

class MULTIAGENT_API FMAHUDSceneListSelectionCoordinator
{
public:
    void HandleGoalClicked(AMAHUD* HUD, const FString& GoalId) const;
    void HandleZoneClicked(AMAHUD* HUD, const FString& ZoneId) const;

private:
    FMAHUDEditRuntimeAdapter RuntimeAdapter;
};
