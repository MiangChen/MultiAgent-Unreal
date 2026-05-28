// MAHUDViewCoordinator.h
// HUD view coordination.

#pragma once

#include "CoreMinimal.h"
#include "MAHUDWidgetCoordinator.h"
#include "../Feedback/MAHUDFeedback.h"

class APlayerController;
class UMAUIManager;
enum class EMAWidgetType : uint8;

class MULTIAGENT_API FMAHUDViewCoordinator
{
public:
    void SetUIManager(UMAUIManager* InUIManager);

    bool ShowWidget(EMAWidgetType WidgetType, bool bSetFocus, const TCHAR* WidgetName) const;
    bool HideWidget(EMAWidgetType WidgetType, const TCHAR* WidgetName) const;
    bool ToggleWidget(EMAWidgetType WidgetType, const TCHAR* WidgetName) const;
    bool IsWidgetVisible(EMAWidgetType WidgetType) const;

    bool IsMouseOverRightSidebar(const APlayerController* PlayerController) const;
    bool IsMouseOverPersistentUI(const APlayerController* PlayerController) const;

    void ShowNotification(const FString& Message, bool bIsError, bool bIsWarning) const;
    void ShowNotification(const FMAHUDNotificationFeedback& Feedback) const;

private:
    bool IsAnyRightSidebarVisible() const;
    bool GetMouseAndViewport(
        const APlayerController* PlayerController,
        float& OutMouseX,
        float& OutMouseY,
        int32& OutViewportSizeX,
        int32& OutViewportSizeY) const;
    bool IsMouseOverSidebarXRange(float MouseX, int32 ViewportSizeX) const;

    UMAUIManager* UIManager = nullptr;
    FMAHUDWidgetCoordinator WidgetCoordinator;
};
