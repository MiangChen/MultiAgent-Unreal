// MAHUDOverlayCoordinator.h
// HUD overlay and edit-mode coordination.

#pragma once

#include "CoreMinimal.h"
#include "MAHUDEditModeIndicatorBuilder.h"
#include "../Infrastructure/MAHUDEditRuntimeAdapter.h"
#include "../../Mode/Application/MAEditWidgetCoordinator.h"

class AMAHUD;
class UMAEditWidget;

class MULTIAGENT_API FMAHUDOverlayCoordinator
{
public:
    void LoadSceneGraphForVisualization(AMAHUD* HUD) const;
    void DrawSceneLabels(AMAHUD* HUD) const;
    void DrawPIPCameras(AMAHUD* HUD) const;
    void StartSceneLabelVisualization(AMAHUD* HUD) const;
    void StopSceneLabelVisualization(AMAHUD* HUD) const;

    void BindEditModeManagerEvents(AMAHUD* HUD) const;
    void BindEditWidgetDelegates(AMAHUD* HUD);
    void DrawEditModeIndicator(AMAHUD* HUD) const;
    void HandleEditModeSelectionChanged(AMAHUD* HUD);
    void HandleTempSceneGraphChanged(AMAHUD* HUD);

    void HandleEditConfirmRequested(AMAHUD* HUD, const FString& JsonContent);
    void HandleEditDeleteRequested(AMAHUD* HUD);
    void HandleEditCreateGoalRequested(AMAHUD* HUD, const FString& Description);
    void HandleEditCreateZoneRequested(AMAHUD* HUD, const FString& Description);
    void HandleEditAddPresetActorRequested(AMAHUD* HUD, const FString& ActorType);
    void HandleEditDeletePOIsRequested(AMAHUD* HUD);
    void HandleEditSetAsGoalRequested(AMAHUD* HUD);
    void HandleEditUnsetAsGoalRequested(AMAHUD* HUD);
    void HandleEditNodeSwitchRequested(AMAHUD* HUD, int32 NodeIndex);

private:
    UMAEditWidget* ResolveEditWidget(AMAHUD* HUD) const;
    void BindEditWidgetDelegates(AMAHUD* HUD, UMAEditWidget* EditWidget) const;

    FMAHUDEditModeIndicatorBuilder EditModeIndicatorBuilder;
    FMAHUDEditRuntimeAdapter RuntimeAdapter;
    FMAEditWidgetCoordinator EditWidgetCoordinator;
};
