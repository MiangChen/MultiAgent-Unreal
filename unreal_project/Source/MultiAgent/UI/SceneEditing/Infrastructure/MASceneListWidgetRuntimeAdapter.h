#pragma once

#include "CoreMinimal.h"
#include "../../../Core/SceneGraph/Feedback/MASceneGraphFeedback.h"

class UUserWidget;

class MULTIAGENT_API FMASceneListWidgetRuntimeAdapter
{
public:
    FMASceneGraphNodesFeedback LoadGoals(const UUserWidget* Context) const;
    FMASceneGraphNodesFeedback LoadZones(const UUserWidget* Context) const;
};
