// MAHUDViewCoordinator.cpp
// HUD view coordination.

#include "MAHUDViewCoordinator.h"
#include "../../Core/MAUIManager.h"
#include "../MAHUDWidget.h"
#include "../MAMainHUDWidget.h"
#include "../../Components/MAMiniMapWidget.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDViewCoordinator, Log, All);

namespace MAHUDViewCoordinatorConstants
{
    static constexpr float SidebarRightMargin = 20.0f;
    static constexpr float SidebarWidth = 480.0f;
    static constexpr float MiniMapLeft = 20.0f;
    static constexpr float MiniMapTop = 20.0f;
    static constexpr float MiniMapSize = 200.0f;
}

void FMAHUDViewCoordinator::SetUIManager(UMAUIManager* InUIManager)
{
    UIManager = InUIManager;
}

bool FMAHUDViewCoordinator::ShowWidget(EMAWidgetType WidgetType, bool bSetFocus, const TCHAR* WidgetName) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDViewCoordinator, Warning, TEXT("ShowWidget(%s): UIManager is null"), WidgetName);
        return false;
    }

    if (UIManager->IsWidgetVisible(WidgetType))
    {
        return true;
    }

    return UIManager->ShowWidget(WidgetType, bSetFocus);
}

bool FMAHUDViewCoordinator::HideWidget(EMAWidgetType WidgetType, const TCHAR* WidgetName) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDViewCoordinator, Warning, TEXT("HideWidget(%s): UIManager is null"), WidgetName);
        return false;
    }

    if (!UIManager->IsWidgetVisible(WidgetType))
    {
        return true;
    }

    return UIManager->HideWidget(WidgetType);
}

bool FMAHUDViewCoordinator::ToggleWidget(EMAWidgetType WidgetType, const TCHAR* WidgetName) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDViewCoordinator, Warning, TEXT("ToggleWidget(%s): UIManager is null"), WidgetName);
        return false;
    }

    UIManager->ToggleWidget(WidgetType);
    return true;
}

bool FMAHUDViewCoordinator::IsWidgetVisible(EMAWidgetType WidgetType) const
{
    return UIManager ? UIManager->IsWidgetVisible(WidgetType) : false;
}

bool FMAHUDViewCoordinator::IsMouseOverRightSidebar(const APlayerController* PlayerController) const
{
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    int32 ViewportSizeX = 0;
    int32 ViewportSizeY = 0;
    if (!GetMouseAndViewport(PlayerController, MouseX, MouseY, ViewportSizeX, ViewportSizeY))
    {
        return false;
    }

    if (!IsAnyRightSidebarVisible())
    {
        return false;
    }

    return IsMouseOverSidebarXRange(MouseX, ViewportSizeX);
}

bool FMAHUDViewCoordinator::IsMouseOverPersistentUI(const APlayerController* PlayerController) const
{
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    int32 ViewportSizeX = 0;
    int32 ViewportSizeY = 0;
    if (!GetMouseAndViewport(PlayerController, MouseX, MouseY, ViewportSizeX, ViewportSizeY))
    {
        return false;
    }

    if (!UIManager)
    {
        return false;
    }

    UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget();
    if (!MainHUDWidget || !MainHUDWidget->IsVisible())
    {
        return false;
    }

    if (IsAnyRightSidebarVisible() && IsMouseOverSidebarXRange(MouseX, ViewportSizeX))
    {
        return true;
    }

    UMAMiniMapWidget* MiniMap = MainHUDWidget->GetMiniMap();
    if (!MiniMap || !MiniMap->IsVisible())
    {
        return false;
    }

    return MouseX >= MAHUDViewCoordinatorConstants::MiniMapLeft &&
           MouseX <= MAHUDViewCoordinatorConstants::MiniMapLeft + MAHUDViewCoordinatorConstants::MiniMapSize &&
           MouseY >= MAHUDViewCoordinatorConstants::MiniMapTop &&
           MouseY <= MAHUDViewCoordinatorConstants::MiniMapTop + MAHUDViewCoordinatorConstants::MiniMapSize;
}

void FMAHUDViewCoordinator::ShowNotification(const FString& Message, bool bIsError, bool bIsWarning) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDViewCoordinator, Warning, TEXT("ShowNotification: UIManager is null"));
        return;
    }

    UMAHUDWidget* HUDWidget = UIManager->GetHUDWidget();
    if (HUDWidget)
    {
        HUDWidget->ApplyNotificationModel(WidgetCoordinator.BuildNotificationModel(Message, bIsError, bIsWarning));
    }
}

void FMAHUDViewCoordinator::ShowNotification(const FMAHUDNotificationFeedback& Feedback) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDViewCoordinator, Warning, TEXT("ShowNotification: UIManager is null"));
        return;
    }

    if (UMAHUDWidget* HUDWidget = UIManager->GetHUDWidget())
    {
        HUDWidget->ApplyNotificationModel(WidgetCoordinator.BuildNotificationModel(Feedback));
    }
}

bool FMAHUDViewCoordinator::IsAnyRightSidebarVisible() const
{
    return UIManager &&
           (UIManager->IsWidgetVisible(EMAWidgetType::SystemLogPanel) ||
            UIManager->IsWidgetVisible(EMAWidgetType::PreviewPanel) ||
            UIManager->IsWidgetVisible(EMAWidgetType::InstructionPanel));
}

bool FMAHUDViewCoordinator::GetMouseAndViewport(
    const APlayerController* PlayerController,
    float& OutMouseX,
    float& OutMouseY,
    int32& OutViewportSizeX,
    int32& OutViewportSizeY) const
{
    if (!PlayerController)
    {
        return false;
    }

    if (!PlayerController->GetMousePosition(OutMouseX, OutMouseY))
    {
        return false;
    }

    PlayerController->GetViewportSize(OutViewportSizeX, OutViewportSizeY);
    return OutViewportSizeX > 0 && OutViewportSizeY > 0;
}

bool FMAHUDViewCoordinator::IsMouseOverSidebarXRange(float MouseX, int32 ViewportSizeX) const
{
    const float SidebarLeft = ViewportSizeX - MAHUDViewCoordinatorConstants::SidebarRightMargin - MAHUDViewCoordinatorConstants::SidebarWidth;
    const float SidebarRight = ViewportSizeX - MAHUDViewCoordinatorConstants::SidebarRightMargin;

    return MouseX >= SidebarLeft && MouseX <= SidebarRight;
}
