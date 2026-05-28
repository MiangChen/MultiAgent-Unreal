// MAUIWidgetLifecycleCoordinator.h
// UIManager Widget 生命周期创建协调器（L1 Application）

#pragma once

#include "CoreMinimal.h"

class UMAUIManager;
class APlayerController;

class MULTIAGENT_API FMAUIWidgetLifecycleCoordinator
{
public:
    void CreateAllWidgets(UMAUIManager* UIManager) const;

private:
    void CreateHUDWidgets(UMAUIManager* UIManager, APlayerController* OwningPC) const;
    void CreatePlannerWidgets(UMAUIManager* UIManager, APlayerController* OwningPC) const;
    void CreateModeWidgets(UMAUIManager* UIManager, APlayerController* OwningPC) const;
    void CreatePanelWidgets(UMAUIManager* UIManager, APlayerController* OwningPC) const;
    void BindRuntimeEventSources(UMAUIManager* UIManager) const;
};
