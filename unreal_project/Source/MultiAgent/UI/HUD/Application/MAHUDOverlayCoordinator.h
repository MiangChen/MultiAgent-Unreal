// MAHUDOverlayCoordinator.h
// HUD 叠加层与 EditMode 协调器
// 封装场景标签、PIP 绘制和 EditMode 绑定/面板更新逻辑

#pragma once

#include "CoreMinimal.h"

class AMAHUD;

class MULTIAGENT_API FMAHUDOverlayCoordinator
{
public:
    void LoadSceneGraphForVisualization(AMAHUD* HUD) const;
    void DrawSceneLabels(AMAHUD* HUD) const;
    void DrawPIPCameras(AMAHUD* HUD) const;
    void StartSceneLabelVisualization(AMAHUD* HUD) const;
    void StopSceneLabelVisualization(AMAHUD* HUD) const;

    void BindEditModeManagerEvents(AMAHUD* HUD) const;
    void BindEditWidgetDelegates(AMAHUD* HUD) const;
    void DrawEditModeIndicator(AMAHUD* HUD) const;
    void HandleEditModeSelectionChanged(AMAHUD* HUD) const;
    void HandleTempSceneGraphChanged(AMAHUD* HUD) const;
};
