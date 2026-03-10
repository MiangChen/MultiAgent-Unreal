#include "MASceneListWidgetRuntimeAdapter.h"

#include "../../../Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.h"
#include "Blueprint/UserWidget.h"

FMASceneGraphNodesFeedback FMASceneListWidgetRuntimeAdapter::LoadGoals(const UUserWidget* Context) const
{
    return FMASceneGraphBootstrap::LoadGoals(Context);
}

FMASceneGraphNodesFeedback FMASceneListWidgetRuntimeAdapter::LoadZones(const UUserWidget* Context) const
{
    return FMASceneGraphBootstrap::LoadZones(Context);
}
