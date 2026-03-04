// MAUIWidgetInteractionCoordinator.cpp
// UIManager Widget 交互与导航协调器实现（L1 Application）

#include "MAUIWidgetInteractionCoordinator.h"
#include "../MAUIManager.h"
#include "../MAHUDStateManager.h"
#include "../../Modal/MATaskGraphModal.h"
#include "../../Modal/MASkillAllocationModal.h"
#include "../../Modal/MADecisionModal.h"
#include "../../SkillAllocation/MASkillAllocationViewer.h"
#include "../../TaskGraph/MATaskPlannerWidget.h"
#include "../../Legacy/MASimpleMainWidget.h"
#include "../../Mode/MAEditWidget.h"
#include "../../../Core/Manager/MATempDataManager.h"
#include "Engine/GameInstance.h"
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

    if (UIManager->HUDStateManager && UIManager->HUDStateManager->IsModalActive())
    {
        EMAModalType ActiveModal = UIManager->HUDStateManager->GetActiveModalType();
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
            UIManager->HUDStateManager->ReturnToNormalHUD();
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

    if (UIManager->TaskPlannerWidget && UIManager->TaskPlannerWidget->IsVisible())
    {
        return true;
    }

    if (UIManager->SkillAllocationViewer && UIManager->SkillAllocationViewer->IsVisible())
    {
        return true;
    }

    if (UIManager->TaskGraphModal && UIManager->TaskGraphModal->IsVisible())
    {
        return true;
    }

    if (UIManager->SkillAllocationModal && UIManager->SkillAllocationModal->IsVisible())
    {
        return true;
    }

    if (UIManager->DecisionModal && UIManager->DecisionModal->IsVisible())
    {
        return true;
    }

    if (UIManager->EditWidget && UIManager->EditWidget->IsVisible())
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

    if (UIManager->SkillAllocationViewer)
    {
        UIManager->SkillAllocationViewer->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillAllocationModal: SkillAllocationViewer hidden"));
    }

    if (UIManager->SkillAllocationModal)
    {
        if (UGameInstance* GameInstance = UIManager->OwningPC ? UIManager->OwningPC->GetGameInstance() : nullptr)
        {
            if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
            {
                FMASkillAllocationData Data;
                if (TempDataMgr->LoadSkillAllocation(Data))
                {
                    UIManager->SkillAllocationModal->LoadSkillAllocation(Data);
                    UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillAllocationModal: Data loaded into modal"));
                }
            }
        }

        UIManager->SkillAllocationModal->SetEditMode(false);
        UIManager->SkillAllocationModal->SetVisibility(ESlateVisibility::Visible);
        UIManager->SkillAllocationModal->PlayShowAnimation();
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillAllocationModal: SkillAllocationModal shown"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("NavigateFromViewerToSkillAllocationModal: SkillAllocationModal is null"));
    }

    if (UIManager->HUDStateManager)
    {
        UIManager->HUDStateManager->TransitionToState(EMAHUDState::ReviewModal, EMAModalType::SkillAllocation);
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

    if (UIManager->TaskPlannerWidget)
    {
        UIManager->TaskPlannerWidget->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: TaskPlannerWidget hidden"));
    }

    if (UIManager->TaskGraphModal)
    {
        if (UGameInstance* GameInstance = UIManager->OwningPC ? UIManager->OwningPC->GetGameInstance() : nullptr)
        {
            if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
            {
                FMATaskGraphData Data;
                if (TempDataMgr->LoadTaskGraph(Data))
                {
                    UIManager->TaskGraphModal->LoadTaskGraph(Data);
                    UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: Data loaded into modal"));
                }
            }
        }

        UIManager->TaskGraphModal->SetEditMode(false);
        UIManager->TaskGraphModal->SetVisibility(ESlateVisibility::Visible);
        UIManager->TaskGraphModal->PlayShowAnimation();
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: TaskGraphModal shown"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("NavigateFromWorkbenchToTaskGraphModal: TaskGraphModal is null"));
    }

    if (UIManager->HUDStateManager)
    {
        UIManager->HUDStateManager->TransitionToState(EMAHUDState::ReviewModal, EMAModalType::TaskGraph);
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
        if (UIManager->TaskPlannerWidget)
        {
            UIManager->TaskPlannerWidget->FocusJsonEditor();
        }
        break;

    case EMAWidgetType::SimpleMain:
        if (UIManager->SimpleMainWidget)
        {
            UIManager->SimpleMainWidget->FocusInputBox();
        }
        break;

    default:
        UIManager->SetInputModeGameAndUI(Widget);
        break;
    }
}
