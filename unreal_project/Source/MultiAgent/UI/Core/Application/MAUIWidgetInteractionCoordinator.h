// MAUIWidgetInteractionCoordinator.h
// UIManager Widget 交互与导航协调器（L1 Application）

#pragma once

#include "CoreMinimal.h"

class UMAUIManager;
enum class EMAWidgetType : uint8;

class MULTIAGENT_API FMAUIWidgetInteractionCoordinator
{
public:
    bool ShowWidget(UMAUIManager* UIManager, EMAWidgetType Type, bool bSetFocus) const;
    bool HideWidget(UMAUIManager* UIManager, EMAWidgetType Type) const;
    void ToggleWidget(UMAUIManager* UIManager, EMAWidgetType Type) const;
    bool IsWidgetVisible(const UMAUIManager* UIManager, EMAWidgetType Type) const;
    bool IsAnyFullscreenWidgetVisible(const UMAUIManager* UIManager) const;

    void NavigateFromViewerToSkillAllocationModal(UMAUIManager* UIManager) const;
    void NavigateFromWorkbenchToTaskGraphModal(UMAUIManager* UIManager) const;
    void SetWidgetFocus(UMAUIManager* UIManager, EMAWidgetType Type) const;
};
