// MAUIStateModalCoordinator.cpp
// UIManager 状态机与模态流程协调器实现（L1 Application）

#include "MAUIStateModalCoordinator.h"
#include "../MAUIManager.h"
#include "../MAHUDStateManager.h"
#include "../../HUD/MAMainHUDWidget.h"
#include "../../Components/MANotificationWidget.h"
#include "../../Modal/MABaseModalWidget.h"
#include "../../Modal/MATaskGraphModal.h"
#include "../../Modal/MASkillAllocationModal.h"
#include "../../Modal/MADecisionModal.h"
#include "../../../Core/TempData/Runtime/MATempDataManager.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void FMAUIStateModalCoordinator::InitializeHUDStateManager(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    UMAHUDStateManager* HUDStateManager = NewObject<UMAHUDStateManager>(UIManager);
    UIManager->SetHUDStateManagerInternal(HUDStateManager);

    if (HUDStateManager)
    {
        BindStateManagerDelegates(UIManager);
        UE_LOG(LogMAUIManager, Log, TEXT("HUDStateManager initialized"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create HUDStateManager"));
    }
}

void FMAUIStateModalCoordinator::CreateModalWidgets(UMAUIManager* UIManager) const
{
    APlayerController* OwningPC = UIManager ? UIManager->GetOwningPlayerController() : nullptr;
    if (!UIManager || !OwningPC)
    {
        UE_LOG(LogMAUIManager, Error, TEXT("CreateModalWidgets: OwningPC is null"));
        return;
    }

    const int32 ModalZOrder = GetModalZOrder();

    UMATaskGraphModal* TaskGraphModal = CreateWidget<UMATaskGraphModal>(OwningPC, UMATaskGraphModal::StaticClass());
    UIManager->SetModalWidgetInternal(EMAModalType::TaskGraph, TaskGraphModal);
    if (TaskGraphModal)
    {
        TaskGraphModal->AddToViewport(ModalZOrder);
        TaskGraphModal->SetVisibility(ESlateVisibility::Collapsed);
        TaskGraphModal->SetModalType(EMAModalType::TaskGraph);

        if (UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager())
        {
            TaskGraphModal->OnConfirmClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalConfirm);
            TaskGraphModal->OnRejectClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalReject);
            TaskGraphModal->OnEditClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalEdit);
        }

        TaskGraphModal->OnHideAnimationComplete.AddDynamic(UIManager, &UMAUIManager::OnModalHideAnimationComplete);

        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            TaskGraphModal->ApplyTheme(ThemeToApply);
        }

        UE_LOG(LogMAUIManager, Log, TEXT("TaskGraphModal created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create TaskGraphModal"));
    }

    UMASkillAllocationModal* SkillAllocationModal = CreateWidget<UMASkillAllocationModal>(OwningPC, UMASkillAllocationModal::StaticClass());
    UIManager->SetModalWidgetInternal(EMAModalType::SkillAllocation, SkillAllocationModal);
    if (SkillAllocationModal)
    {
        SkillAllocationModal->AddToViewport(ModalZOrder);
        SkillAllocationModal->SetVisibility(ESlateVisibility::Collapsed);
        SkillAllocationModal->SetModalType(EMAModalType::SkillAllocation);

        if (UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager())
        {
            SkillAllocationModal->OnConfirmClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalConfirm);
            SkillAllocationModal->OnRejectClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalReject);
            SkillAllocationModal->OnEditClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalEdit);
        }

        SkillAllocationModal->OnHideAnimationComplete.AddDynamic(UIManager, &UMAUIManager::OnModalHideAnimationComplete);

        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            SkillAllocationModal->ApplyTheme(ThemeToApply);
        }

        UE_LOG(LogMAUIManager, Log, TEXT("SkillAllocationModal created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create SkillAllocationModal"));
    }

    UMADecisionModal* DecisionModal = CreateWidget<UMADecisionModal>(OwningPC, UMADecisionModal::StaticClass());
    UIManager->SetModalWidgetInternal(EMAModalType::Decision, DecisionModal);
    if (DecisionModal)
    {
        DecisionModal->AddToViewport(ModalZOrder);
        DecisionModal->SetVisibility(ESlateVisibility::Collapsed);
        DecisionModal->SetModalType(EMAModalType::Decision);

        if (UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager())
        {
            DecisionModal->OnConfirmClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalConfirm);
            DecisionModal->OnRejectClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalReject);
        }

        DecisionModal->OnDecisionConfirmed.AddDynamic(UIManager, &UMAUIManager::OnDecisionModalConfirmed);
        DecisionModal->OnDecisionRejected.AddDynamic(UIManager, &UMAUIManager::OnDecisionModalRejected);
        DecisionModal->OnHideAnimationComplete.AddDynamic(UIManager, &UMAUIManager::OnModalHideAnimationComplete);

        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            DecisionModal->ApplyTheme(ThemeToApply);
        }

        UE_LOG(LogMAUIManager, Log, TEXT("DecisionModal created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create DecisionModal"));
    }
}

void FMAUIStateModalCoordinator::BindStateManagerDelegates(UMAUIManager* UIManager) const
{
    UMAHUDStateManager* HUDStateManager = UIManager ? UIManager->GetHUDStateManager() : nullptr;
    if (!UIManager || !HUDStateManager)
    {
        return;
    }

    HUDStateManager->OnStateChanged.AddDynamic(UIManager, &UMAUIManager::OnHUDStateChanged);
    HUDStateManager->OnNotificationReceived.AddDynamic(UIManager, &UMAUIManager::OnNotificationReceived);
    HUDStateManager->OnModalConfirmed.AddDynamic(UIManager, &UMAUIManager::OnModalConfirmed);
    HUDStateManager->OnModalRejected.AddDynamic(UIManager, &UMAUIManager::OnModalRejected);
    HUDStateManager->OnModalEditRequested.AddDynamic(UIManager, &UMAUIManager::OnModalEditRequested);

    UE_LOG(LogMAUIManager, Log, TEXT("HUDStateManager delegates bound"));
}

void FMAUIStateModalCoordinator::HandleHUDStateChanged(UMAUIManager* UIManager, EMAHUDState OldState, EMAHUDState NewState) const
{
    if (!UIManager)
    {
        return;
    }

    UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager();
    UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget();

    UE_LOG(LogMAUIManager, Log, TEXT("HUD State changed: %s -> %s"),
        *UEnum::GetValueAsString(OldState),
        *UEnum::GetValueAsString(NewState));

    switch (NewState)
    {
    case EMAHUDState::NormalHUD:
        HideCurrentModal(UIManager);
        UIManager->HideWidget(EMAWidgetType::TaskPlanner);
        UIManager->HideWidget(EMAWidgetType::SkillAllocation);

        if (MainHUDWidget && MainHUDWidget->GetNotification())
        {
            MainHUDWidget->GetNotification()->HideNotification();
        }

        UIManager->SetInputModeGameOnly();
        break;

    case EMAHUDState::NotificationPending:
        break;

    case EMAHUDState::ReviewModal:
        if (MainHUDWidget && MainHUDWidget->GetNotification())
        {
            if (HUDStateManager && HUDStateManager->GetActiveModalType() != EMAModalType::Decision)
            {
                MainHUDWidget->GetNotification()->HideNotification();
            }
        }

        if (HUDStateManager)
        {
            ShowModal(UIManager, HUDStateManager->GetActiveModalType(), false);
        }
        break;

    case EMAHUDState::EditingModal:
        if (MainHUDWidget && MainHUDWidget->GetNotification())
        {
            if (HUDStateManager && HUDStateManager->GetActiveModalType() != EMAModalType::Decision)
            {
                MainHUDWidget->GetNotification()->HideNotification();
            }
        }

        if (HUDStateManager)
        {
            const EMAModalType ModalType = HUDStateManager->GetActiveModalType();
            if (ModalType == EMAModalType::Decision)
            {
                if (UMADecisionModal* DecisionModal = UIManager->GetDecisionModal())
                {
                    DecisionModal->SetEditMode(true);
                    DecisionModal->SetVisibility(ESlateVisibility::Visible);
                    DecisionModal->PlayShowAnimation();
                    UIManager->SetInputModeGameAndUI(DecisionModal);
                    UE_LOG(LogMAUIManager, Log, TEXT("OnHUDStateChanged: Showing DecisionModal for decision request"));
                }
            }
        }
        break;
    }
}

void FMAUIStateModalCoordinator::HandleNotificationReceived(UMAUIManager* UIManager, EMANotificationType Type) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnNotificationReceived: Type=%s"),
        *UEnum::GetValueAsString(Type));

    if (UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget())
    {
        UMANotificationWidget* NotificationWidget = MainHUDWidget->GetNotification();
        if (NotificationWidget)
        {
            UE_LOG(LogMAUIManager, Log, TEXT("OnNotificationReceived: Calling NotificationWidget->ShowNotification"));
            NotificationWidget->ShowNotification(Type);
        }
        else
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("OnNotificationReceived: NotificationWidget is null"));
        }
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnNotificationReceived: MainHUDWidget is null"));
    }
}

void FMAUIStateModalCoordinator::HandleModalConfirmed(EMAModalType ModalType) const
{
    UE_LOG(LogMAUIManager, Log, TEXT("Modal confirmed: %s"),
        *UEnum::GetValueAsString(ModalType));
}

void FMAUIStateModalCoordinator::HandleModalRejected(EMAModalType ModalType) const
{
    UE_LOG(LogMAUIManager, Log, TEXT("Modal rejected: %s"),
        *UEnum::GetValueAsString(ModalType));
}

void FMAUIStateModalCoordinator::HandleModalEditRequested(UMAUIManager* UIManager, EMAModalType ModalType) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("Modal edit requested: %s"),
        *UEnum::GetValueAsString(ModalType));

    HideCurrentModal(UIManager);

    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        UIManager->ShowWidget(EMAWidgetType::TaskPlanner, true);
        UE_LOG(LogMAUIManager, Log, TEXT("Showing TaskPlannerWidget for editing"));
        break;

    case EMAModalType::SkillAllocation:
        UIManager->ShowWidget(EMAWidgetType::SkillAllocation, true);
        UE_LOG(LogMAUIManager, Log, TEXT("Showing SkillAllocationViewer for editing"));
        break;

    default:
        UE_LOG(LogMAUIManager, Warning, TEXT("Unknown modal type for edit: %s"),
            *UEnum::GetValueAsString(ModalType));
        break;
    }
}

void FMAUIStateModalCoordinator::HandleModalHideAnimationComplete(UMAUIManager* UIManager) const
{
    UMAHUDStateManager* HUDStateManager = UIManager ? UIManager->GetHUDStateManager() : nullptr;
    if (!UIManager || !HUDStateManager || !HUDStateManager->IsModalActive())
    {
        return;
    }

    if (HUDStateManager->IsInEditMode())
    {
        UE_LOG(LogMAUIManager, Log, TEXT("OnModalHideAnimationComplete: In EditingModal state, skipping ReturnToNormalHUD"));
        return;
    }

    if (HUDStateManager->GetActiveModalType() == EMAModalType::Decision)
    {
        UE_LOG(LogMAUIManager, Log, TEXT("OnModalHideAnimationComplete: Decision Modal closed via X button, keeping decision state and notification active"));
        HUDStateManager->TransitionToState(EMAHUDState::NotificationPending);
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnModalHideAnimationComplete: Returning HUDStateManager to NormalHUD"));
    HUDStateManager->ReturnToNormalHUD();
}

void FMAUIStateModalCoordinator::ShowModal(UMAUIManager* UIManager, EMAModalType ModalType, bool bEditMode) const
{
    if (!UIManager)
    {
        return;
    }

    HideCurrentModal(UIManager);

    UMABaseModalWidget* Modal = UIManager->GetModalByType(ModalType);
    if (!Modal)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("ShowModal: Modal not found for type %s"),
            *UEnum::GetValueAsString(ModalType));
        return;
    }

    LoadDataIntoModal(UIManager, ModalType);
    Modal->SetEditMode(bEditMode);
    Modal->SetVisibility(ESlateVisibility::Visible);
    Modal->PlayShowAnimation();
    UIManager->SetInputModeGameAndUI(Modal);

    UE_LOG(LogMAUIManager, Log, TEXT("Modal shown: %s (EditMode: %s)"),
        *UEnum::GetValueAsString(ModalType),
        bEditMode ? TEXT("true") : TEXT("false"));
}

void FMAUIStateModalCoordinator::HideCurrentModal(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    if (UMATaskGraphModal* TaskGraphModal = UIManager->GetTaskGraphModal(); TaskGraphModal && TaskGraphModal->IsVisible())
    {
        TaskGraphModal->PlayHideAnimation();
    }

    if (UMASkillAllocationModal* SkillAllocationModal = UIManager->GetSkillAllocationModal(); SkillAllocationModal && SkillAllocationModal->IsVisible())
    {
        SkillAllocationModal->PlayHideAnimation();
    }

    if (UMADecisionModal* DecisionModal = UIManager->GetDecisionModal(); DecisionModal && DecisionModal->IsVisible())
    {
        DecisionModal->PlayHideAnimation();
    }
}

void FMAUIStateModalCoordinator::LoadDataIntoModal(UMAUIManager* UIManager, EMAModalType ModalType) const
{
    if (!UIManager)
    {
        return;
    }

    APlayerController* OwningPC = UIManager->GetOwningPlayerController();
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    UMATempDataManager* TempDataMgr = GameInstance ? GameInstance->GetSubsystem<UMATempDataManager>() : nullptr;

    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        if (UMATaskGraphModal* TaskGraphModal = UIManager->GetTaskGraphModal())
        {
            if (TempDataMgr && TempDataMgr->TaskGraphFileExists())
            {
                FMATaskGraphData Data;
                if (TempDataMgr->LoadTaskGraph(Data))
                {
                    TaskGraphModal->LoadTaskGraph(Data);
                    UE_LOG(LogMAUIManager, Log, TEXT("TaskGraphModal: Loaded data from temp file (%d nodes, %d edges)"),
                        Data.Nodes.Num(), Data.Edges.Num());
                    return;
                }
            }

            FMATaskGraphData EmptyData;
            TaskGraphModal->LoadTaskGraph(EmptyData);
            UE_LOG(LogMAUIManager, Log, TEXT("TaskGraphModal: No data available, showing empty modal (waiting for backend data)"));
        }
        break;

    case EMAModalType::SkillAllocation:
        if (UMASkillAllocationModal* SkillAllocationModal = UIManager->GetSkillAllocationModal())
        {
            if (TempDataMgr && TempDataMgr->SkillAllocationFileExists())
            {
                FMASkillAllocationData Data;
                if (TempDataMgr->LoadSkillAllocation(Data))
                {
                    SkillAllocationModal->LoadSkillAllocation(Data);
                    UE_LOG(LogMAUIManager, Log, TEXT("SkillAllocationModal: Loaded data from temp file (%d time steps)"),
                        Data.Data.Num());
                    return;
                }
            }

            FMASkillAllocationData EmptyData;
            SkillAllocationModal->LoadSkillAllocation(EmptyData);
            UE_LOG(LogMAUIManager, Log, TEXT("SkillAllocationModal: No data available, showing empty modal (waiting for backend data)"));
        }
        break;

    default:
        break;
    }
}

int32 FMAUIStateModalCoordinator::GetModalZOrder() const
{
    return 20;
}
