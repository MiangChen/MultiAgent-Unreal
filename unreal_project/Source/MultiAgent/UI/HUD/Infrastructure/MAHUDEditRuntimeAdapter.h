#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAHUDEditModeIndicatorModel.h"

class AMAHUD;
class UMASceneListWidget;
struct FMASceneGraphNode;

class MULTIAGENT_API FMAHUDEditRuntimeAdapter
{
public:
    bool LoadSceneGraphNodes(AMAHUD* HUD, TArray<FMASceneGraphNode>& OutNodes) const;
    void DrawSceneLabels(AMAHUD* HUD, const TArray<FMASceneGraphNode>& Nodes) const;
    void DrawPIPCameras(AMAHUD* HUD) const;
    bool BindEditModeSelectionChanged(AMAHUD* HUD) const;
    void AssignSceneListEditModeManager(AMAHUD* HUD, UMASceneListWidget* SceneListWidget) const;
    bool BuildEditModeIndicatorModel(AMAHUD* HUD, FMAHUDEditModeIndicatorModel& OutModel) const;
    bool SelectGoalById(AMAHUD* HUD, const FString& GoalId) const;
    bool SelectZoneById(AMAHUD* HUD, const FString& ZoneId) const;

private:
    class UWorld* ResolveWorld(const AMAHUD* HUD) const;
    class UMAEditModeManager* ResolveEditModeManager(const AMAHUD* HUD) const;
    class UMASceneGraphManager* ResolveSceneGraphManager(const AMAHUD* HUD) const;
    class UMAPIPCameraManager* ResolvePIPCameraManager(const AMAHUD* HUD) const;
};
