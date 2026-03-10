// L2 selection HUD orchestration.

#include "MASelectionHUDCoordinator.h"

#include "../MASelectionHUD.h"

FMASelectionHUDFrameModel FMASelectionHUDCoordinator::BuildFrameModel(const AMASelectionHUD* HUD, bool bShowAgentCircles) const
{
    FMASelectionHUDFrameModel Model;
    Model.bHasFullscreenWidget = HUD ? HUD->RuntimeIsAnyFullscreenWidgetVisible() : false;
    Model.bHasBlockingVisibleWidget = HUD ? HUD->RuntimeHasBlockingVisibleWidget() : false;

    if (bShowAgentCircles && !Model.bHasFullscreenWidget)
    {
        HUD->RuntimeBuildCircleEntries(Model.CircleEntries);
        HUD->RuntimeBuildStatusTextEntries(Model.StatusEntries);
    }

    HUD->RuntimeBuildControlGroupEntries(Model.ControlGroupEntries);
    return Model;
}
