// MAHUDWidgetCoordinator.h
// HUD widget coordination.

#pragma once

#include "CoreMinimal.h"

class UMAUIManager;
struct FMATaskGraphData;

class MULTIAGENT_API FMAHUDWidgetCoordinator
{
public:
    void LoadTaskGraph(UMAUIManager* UIManager, const FMATaskGraphData& Data) const;

    void ShowDirectControlIndicator(UMAUIManager* UIManager, const FString& AgentLabel) const;
    void HideDirectControlIndicator(UMAUIManager* UIManager) const;
    bool IsDirectControlIndicatorVisible(const UMAUIManager* UIManager) const;
};
