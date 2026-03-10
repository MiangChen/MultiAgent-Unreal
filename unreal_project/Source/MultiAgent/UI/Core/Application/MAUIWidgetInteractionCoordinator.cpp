// MAUIWidgetInteractionCoordinator.cpp
// UIManager Widget 交互与导航协调器实现（L1 Application）

#include "MAUIWidgetInteractionCoordinator.h"
#include "../MAUIManager.h"
#include "../MAHUDStateManager.h"
#include "../../Modal/MABaseModalWidget.h"
#include "../../Modal/MATaskGraphModal.h"
#include "../../Modal/MASkillAllocationModal.h"
#include "../../Modal/MADecisionModal.h"
#include "../../SkillAllocation/MASkillAllocationViewer.h"
#include "../../TaskGraph/MATaskPlannerWidget.h"
#include "../../Mode/MAEditWidget.h"
#include "Blueprint/UserWidget.h"

bool FMAUIWidgetInteractionCoordinator::ShowWidget(UMAUIManager* UIManager, EMAWidgetType Type, bool bSetFocus) const
{
    if (!UIManager)
    {
        return false;
    }

    UUserWidget* Widget = UIManager->GetWidget(Type);
    if (!Widget)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("ShowWidget: Widget type %d not found"), static_cast<int32>(Type));
        return false;
    }

    if (Widget->IsVisible())
    {
        if (bSetFocus)
        {
            SetWidgetFocus(UIManager, Type);
        }
        return true;
    }

    Widget->SetVisibility(ESlateVisibility::Visible);

    if (bSetFocus)
    {
        UIManager->SetInputModeGameAndUI(Widget);
    }

    UE_LOG(LogMAUIManager, Log, TEXT("Widget type %d shown"), static_cast<int32>(Type));
    return true;
}

bool FMAUIWidgetInteractionCoordinator::HideWidget(UMAUIManager* UIManager, EMAWidgetType Type) const
{
    if (!UIManager)
    {
        return false;
    }

    UUserWidget* Widget = UIManager->GetWidget(Type);
    if (!Widget)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("HideWidget: Widget type %d not found"), static_cast<int32>(Type));
        return false;
    }

    if (!Widget->IsVisible())
    {
        return true;
    }

    Widget->SetVisibility(ESlateVisibility::Collapsed);
    UIManager->SetInputModeGameOnly();

    UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager();
    if (HUDStateManager && HUDStateManager->IsModalActive())
    {
        EMAModalType ActiveModal = HUDStateManager->GetActiveModalType();
        bool bShouldReturnToNormal = false;

        switch (Type)
        {
        case EMAWidgetType::TaskPlanner:
            bShouldReturnToNormal = (ActiveModal == EMAModalType::TaskGraph);
            break;
        case EMAWidgetType::SkillAllocation:
            bShouldReturnToNormal = (ActiveModal == EMAModalType::SkillAllocation);
            break;
        default:
            break;
        }

        if (bShouldReturnToNormal)
        {
            UE_LOG(LogMAUIManager, Log, TEXT("HideWidget: Returning HUDStateManager to NormalHUD (was editing %s)"),
                *UEnum::GetValueAsString(ActiveModal));
            HUDStateManager->ReturnToNormalHUD();
        }
    }

    UE_LOG(LogMAUIManager, Log, TEXT("Widget type %d hidden"), static_cast<int32>(Type));
    return true;
}

void FMAUIWidgetInteractionCoordinator::ToggleWidget(UMAUIManager* UIManager, EMAWidgetType Type) const
{
    if (!UIManager)
    {
        return;
    }

    if (IsWidgetVisible(UIManager, Type))
    {
        HideWidget(UIManager, Type);
    }
    else
    {
        ShowWidget(UIManager, Type, true);
    }
}

bool FMAUIWidgetInteractionCoordinator::IsWidgetVisible(const UMAUIManager* UIManager, EMAWidgetType Type) const
{
    if (!UIManager)
    {
        return false;
    }

    UUserWidget* Widget = UIManager->GetWidget(Type);
    return Widget ? Widget->IsVisible() : false;
}

bool FMAUIWidgetInteractionCoordinator::IsAnyFullscreenWidgetVisible(const UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return false;
    }

    if (const UMATaskPlannerWidget* TaskPlannerWidget = UIManager->GetTaskPlannerWidget();
        TaskPlannerWidget && TaskPlannerWidget->IsVisible())
    {
        return true;
    }

    if (const UMASkillAllocationViewer* SkillAllocationViewer = UIManager->GetSkillAllocationViewer();
        SkillAllocationViewer && SkillAllocationViewer->IsVisible())
    {
        return true;
    }

    if (const UMABaseModalWidget* TaskGraphModal = UIManager->GetModalByType(EMAModalType::TaskGraph);
        TaskGraphModal && TaskGraphModal->IsVisible())
    {
        return true;
    }

    if (const UMABaseModalWidget* SkillAllocationModal = UIManager->GetModalByType(EMAModalType::SkillAllocation);
        SkillAllocationModal && SkillAllocationModal->IsVisible())
    {
        return true;
    }

    if (const UMADecisionModal* DecisionModal = UIManager->GetDecisionModal();
        DecisionModal && DecisionModal->IsVisible())
    {
        return true;
    }

    if (const UMAEditWidget* EditWidget = UIManager->GetEditWidget();
        EditWidget && EditWidget->IsVisible())
    {
        return true;
    }

    return false;
}

void FMAUIWidgetInteractionCoordinator::NavigateFromViewerToSkillAllocationModal(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillAllocationModal: Starting navigation"));

    if (UMASkillAllocationViewer* SkillAllocationViewer = UIManager->GetSkillAllocationViewer())
    {
        SkillAllocationViewer->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillAllocationModal: SkillAllocationViewer hidden"));
    }

    if (UMASkillAllocationModal* SkillAllocationModal = UIManager->GetSkillAllocationModal())
    {
        FString RuntimeError;
        FMASkillAllocationData Data;
        if (UIManager->LoadSkillAllocationDataInternal(Data, RuntimeError))
        {
            SkillAllocationModal->LoadSkillAllocation(Data);
            UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillAllocationModal: Data loaded into modal"));
        }

        SkillAllocationModal->SetEditMode(false);
        SkillAllocationModal->SetVisibility(ESlateVisibility::Visible);
        SkillAllocationModal->PlayShowAnimation();
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillAllocationModal: SkillAllocationModal shown"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("NavigateFromViewerToSkillAllocationModal: SkillAllocationModal is null"));
    }

    if (UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager())
    {
        HUDStateManager->TransitionToState(EMAHUDState::ReviewModal, EMAModalType::SkillAllocation);
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillListModal: State transitioned back to ReviewModal"));
    }
}

void FMAUIWidgetInteractionCoordinator::NavigateFromWorkbenchToTaskGraphModal(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: Starting navigation"));

    if (UMATaskPlannerWidget* TaskPlannerWidget = UIManager->GetTaskPlannerWidget())
    {
        TaskPlannerWidget->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: TaskPlannerWidget hidden"));
    }

    if (UMATaskGraphModal* TaskGraphModal = UIManager->GetTaskGraphModal())
    {
        FString RuntimeError;
        FMATaskGraphData Data;
        if (UIManager->LoadTaskGraphDataInternal(Data, RuntimeError))
        {
            TaskGraphModal->LoadTaskGraph(Data);
            UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: Data loaded into modal"));
        }

        TaskGraphModal->SetEditMode(false);
        TaskGraphModal->SetVisibility(ESlateVisibility::Visible);
        TaskGraphModal->PlayShowAnimation();
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: TaskGraphModal shown"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("NavigateFromWorkbenchToTaskGraphModal: TaskGraphModal is null"));
    }

    if (UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager())
    {
        HUDStateManager->TransitionToState(EMAHUDState::ReviewModal, EMAModalType::TaskGraph);
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: State transitioned back to ReviewModal"));
    }
}

void FMAUIWidgetInteractionCoordinator::SetWidgetFocus(UMAUIManager* UIManager, EMAWidgetType Type) const
{
    if (!UIManager)
    {
        return;
    }

    UUserWidget* Widget = UIManager->GetWidget(Type);
    if (!Widget)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("SetWidgetFocus: Widget type %d not found"), static_cast<int32>(Type));
        return;
    }

    switch (Type)
    {
    case EMAWidgetType::TaskPlanner:
        if (UMATaskPlannerWidget* TaskPlannerWidget = UIManager->GetTaskPlannerWidget())
        {
            TaskPlannerWidget->FocusJsonEditor();
        }
        break;

    default:
        UIManager->SetInputModeGameAndUI(Widget);
        break;
    }
}
