// MAHUDPanelCoordinator.cpp
// HUD 面板控制协调器实现

#include "MAHUDPanelCoordinator.h"
#include "../MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../Mode/MAEditWidget.h"
#include "../../Mode/MAModifyWidget.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDPanelCoordinator, Log, All);

namespace MAHUDPanelCoordinatorConstants
{
    static constexpr float EditWidgetRightMargin = 20.0f;
    static constexpr float EditWidgetWidth = 380.0f;
    static constexpr float EditWidgetTop = 60.0f;
    static constexpr float EditWidgetHeight = 600.0f;
}

void FMAHUDPanelCoordinator::HandleMainUIToggled(AMAHUD* HUD, bool bVisible) const
{
    if (bVisible)
    {
        ShowMainUI(HUD);
    }
    else
    {
        HideMainUI(HUD);
    }
}

void FMAHUDPanelCoordinator::HandleRefocusMainUI(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        return;
    }

    if (HUD->bMainUIVisible)
    {
        HUD->UIManager->SetWidgetFocus(EMAWidgetType::TaskPlanner);
    }
}

void FMAHUDPanelCoordinator::ToggleMainUI(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    if (HUD->bMainUIVisible)
    {
        HideMainUI(HUD);
    }
    else
    {
        ShowMainUI(HUD);
    }
}

void FMAHUDPanelCoordinator::ShowMainUI(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        UE_LOG(LogMAHUDPanelCoordinator, Warning, TEXT("ShowMainUI: UIManager is null"));
        return;
    }

    if (HUD->bMainUIVisible)
    {
        HUD->UIManager->SetWidgetFocus(EMAWidgetType::TaskPlanner);
        return;
    }

    if (HUD->UIManager->ShowWidget(EMAWidgetType::TaskPlanner, true))
    {
        HUD->bMainUIVisible = true;
    }
}

void FMAHUDPanelCoordinator::HideMainUI(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        UE_LOG(LogMAHUDPanelCoordinator, Warning, TEXT("HideMainUI: UIManager is null"));
        return;
    }

    if (!HUD->bMainUIVisible)
    {
        return;
    }

    if (HUD->UIManager->HideWidget(EMAWidgetType::TaskPlanner))
    {
        HUD->bMainUIVisible = false;
    }
}

void FMAHUDPanelCoordinator::ToggleSemanticMap(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        UE_LOG(LogMAHUDPanelCoordinator, Warning, TEXT("ToggleSemanticMap: UIManager is null"));
        return;
    }

    HUD->UIManager->ToggleWidget(EMAWidgetType::SemanticMap);
}

void FMAHUDPanelCoordinator::ToggleSkillAllocationViewer(AMAHUD* HUD) const
{
    if (IsSkillAllocationViewerVisible(HUD))
    {
        HideSkillAllocationViewer(HUD);
    }
    else
    {
        ShowSkillAllocationViewer(HUD);
    }
}

void FMAHUDPanelCoordinator::ShowSkillAllocationViewer(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        UE_LOG(LogMAHUDPanelCoordinator, Warning, TEXT("ShowSkillAllocationViewer: UIManager is null"));
        return;
    }

    if (IsSkillAllocationViewerVisible(HUD))
    {
        return;
    }

    HUD->UIManager->ShowWidget(EMAWidgetType::SkillAllocation, true);
}

void FMAHUDPanelCoordinator::HideSkillAllocationViewer(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        UE_LOG(LogMAHUDPanelCoordinator, Warning, TEXT("HideSkillAllocationViewer: UIManager is null"));
        return;
    }

    if (!IsSkillAllocationViewerVisible(HUD))
    {
        return;
    }

    HUD->UIManager->HideWidget(EMAWidgetType::SkillAllocation);
}

bool FMAHUDPanelCoordinator::IsSkillAllocationViewerVisible(const AMAHUD* HUD) const
{
    return HUD && HUD->UIManager && HUD->UIManager->IsWidgetVisible(EMAWidgetType::SkillAllocation);
}

void FMAHUDPanelCoordinator::ShowModifyWidget(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        UE_LOG(LogMAHUDPanelCoordinator, Warning, TEXT("ShowModifyWidget: UIManager is null"));
        return;
    }

    if (IsModifyWidgetVisible(HUD))
    {
        return;
    }

    HUD->UIManager->ShowWidget(EMAWidgetType::Modify, true);
    HUD->StartSceneLabelVisualization();
}

void FMAHUDPanelCoordinator::HideModifyWidget(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        UE_LOG(LogMAHUDPanelCoordinator, Warning, TEXT("HideModifyWidget: UIManager is null"));
        return;
    }

    if (!IsModifyWidgetVisible(HUD))
    {
        return;
    }

    HUD->StopSceneLabelVisualization();
    HUD->UIManager->HideWidget(EMAWidgetType::Modify);

    if (UMAModifyWidget* ModifyWidget = HUD->UIManager->GetModifyWidget())
    {
        ModifyWidget->ClearSelection();
    }
}

bool FMAHUDPanelCoordinator::IsModifyWidgetVisible(const AMAHUD* HUD) const
{
    return HUD && HUD->UIManager && HUD->UIManager->IsWidgetVisible(EMAWidgetType::Modify);
}

void FMAHUDPanelCoordinator::ShowEditWidget(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        UE_LOG(LogMAHUDPanelCoordinator, Warning, TEXT("ShowEditWidget: UIManager is null"));
        return;
    }

    if (IsEditWidgetVisible(HUD))
    {
        return;
    }

    HUD->UIManager->ShowWidget(EMAWidgetType::Edit, true);
}

void FMAHUDPanelCoordinator::HideEditWidget(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        UE_LOG(LogMAHUDPanelCoordinator, Warning, TEXT("HideEditWidget: UIManager is null"));
        return;
    }

    if (!IsEditWidgetVisible(HUD))
    {
        return;
    }

    HUD->UIManager->HideWidget(EMAWidgetType::Edit);

    if (UMAEditWidget* EditWidget = HUD->UIManager->GetEditWidget())
    {
        EditWidget->ClearSelection();
    }
}

bool FMAHUDPanelCoordinator::IsEditWidgetVisible(const AMAHUD* HUD) const
{
    return HUD && HUD->UIManager && HUD->UIManager->IsWidgetVisible(EMAWidgetType::Edit);
}

bool FMAHUDPanelCoordinator::IsMouseOverEditWidget(const AMAHUD* HUD) const
{
    if (!IsEditWidgetVisible(HUD) || !HUD)
    {
        return false;
    }

    APlayerController* PlayerController = HUD->GetOwningPlayerController();
    if (!PlayerController)
    {
        return false;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!PlayerController->GetMousePosition(MouseX, MouseY))
    {
        return false;
    }

    int32 ViewportSizeX = 0;
    int32 ViewportSizeY = 0;
    PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);

    UMAEditWidget* EditWidget = HUD->UIManager ? HUD->UIManager->GetEditWidget() : nullptr;
    if (!EditWidget)
    {
        return false;
    }

    const float Left = ViewportSizeX - MAHUDPanelCoordinatorConstants::EditWidgetRightMargin - MAHUDPanelCoordinatorConstants::EditWidgetWidth;
    const float Right = ViewportSizeX - MAHUDPanelCoordinatorConstants::EditWidgetRightMargin;
    const float Top = MAHUDPanelCoordinatorConstants::EditWidgetTop;
    const float Bottom = MAHUDPanelCoordinatorConstants::EditWidgetTop + MAHUDPanelCoordinatorConstants::EditWidgetHeight;

    return MouseX >= Left && MouseX <= Right && MouseY >= Top && MouseY <= Bottom;
}
