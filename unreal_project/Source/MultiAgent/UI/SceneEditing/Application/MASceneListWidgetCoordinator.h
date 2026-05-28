#pragma once

#include "CoreMinimal.h"
#include "../Domain/MASceneListWidgetModels.h"

class UMASceneListWidget;

class MULTIAGENT_API FMASceneListWidgetCoordinator
{
public:
    FMASceneListSectionModel BuildGoalSection(UMASceneListWidget* Widget) const;
    FMASceneListSectionModel BuildZoneSection(UMASceneListWidget* Widget) const;
};
