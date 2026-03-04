// MAHUDWidgetCoordinator.h
// HUD Widget 协调器
// 封装 TaskGraph 与 DirectControl 指示器相关逻辑

#pragma once

#include "CoreMinimal.h"

class AMACharacter;
class UMAUIManager;
struct FMATaskGraphData;

class MULTIAGENT_API FMAHUDWidgetCoordinator
{
public:
    void LoadTaskGraph(UMAUIManager* UIManager, const FMATaskGraphData& Data) const;

    void ShowDirectControlIndicator(UMAUIManager* UIManager, AMACharacter* Agent) const;
    void HideDirectControlIndicator(UMAUIManager* UIManager) const;
    bool IsDirectControlIndicatorVisible(const UMAUIManager* UIManager) const;
};
