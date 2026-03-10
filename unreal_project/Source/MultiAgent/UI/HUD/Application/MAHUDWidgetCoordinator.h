// MAHUDWidgetCoordinator.h
// HUD widget coordination.

#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAHUDWidgetModels.h"

class UMAUIManager;
struct FMATaskGraphData;

class MULTIAGENT_API FMAHUDWidgetCoordinator
{
public:
    void LoadTaskGraph(UMAUIManager* UIManager, const FMATaskGraphData& Data) const;

    void ShowDirectControlIndicator(UMAUIManager* UIManager, const FString& AgentLabel) const;
    void HideDirectControlIndicator(UMAUIManager* UIManager) const;
    bool IsDirectControlIndicatorVisible(const UMAUIManager* UIManager) const;
    FMAHUDWidgetEditModeViewModel BuildEditModeViewModel(
        bool bVisible,
        const TArray<FString>& POIInfos,
        const TArray<FString>& GoalInfos,
        const TArray<FString>& ZoneInfos) const;
    FMAHUDWidgetNotificationModel BuildNotificationModel(const FString& Message, bool bIsError, bool bIsWarning) const;

private:
    FString BuildPrefixedListText(const TCHAR* Prefix, const TArray<FString>& Infos) const;
};
