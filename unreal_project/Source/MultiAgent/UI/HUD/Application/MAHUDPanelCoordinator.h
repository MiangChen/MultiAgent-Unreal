// MAHUDPanelCoordinator.h
// HUD panel visibility coordination.

#pragma once

#include "CoreMinimal.h"
#include "MAHUDModeWidgetLifecycleCoordinator.h"

class AMAHUD;

class MULTIAGENT_API FMAHUDPanelCoordinator
{
public:
    void HandleMainUIToggled(AMAHUD* HUD, bool bVisible) const;
    void HandleRefocusMainUI(AMAHUD* HUD) const;

    void ToggleMainUI(AMAHUD* HUD) const;
    void ShowMainUI(AMAHUD* HUD) const;
    void HideMainUI(AMAHUD* HUD) const;
    void ToggleSemanticMap(AMAHUD* HUD) const;

    void ToggleSkillAllocationViewer(AMAHUD* HUD) const;
    void ShowSkillAllocationViewer(AMAHUD* HUD) const;
    void HideSkillAllocationViewer(AMAHUD* HUD) const;
    bool IsSkillAllocationViewerVisible(const AMAHUD* HUD) const;

    void ShowModifyWidget(AMAHUD* HUD) const;
    void HideModifyWidget(AMAHUD* HUD) const;
    bool IsModifyWidgetVisible(const AMAHUD* HUD) const;

    void ShowEditWidget(AMAHUD* HUD) const;
    void HideEditWidget(AMAHUD* HUD) const;
    bool IsEditWidgetVisible(const AMAHUD* HUD) const;
    bool IsMouseOverEditWidget(const AMAHUD* HUD) const;

private:
    FMAHUDModeWidgetLifecycleCoordinator ModeWidgetLifecycleCoordinator;
};
