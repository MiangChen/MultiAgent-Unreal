// MAHUDBackendCoordinator.cpp
// HUD backend interaction coordination.

#include "MAHUDBackendCoordinator.h"
#include "../MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../Core/MAHUDStateManager.h"
#include "../../TaskGraph/MATaskPlannerWidget.h"
#include "../../Modal/MATaskGraphModal.h"
#include "../../Modal/MASkillAllocationModal.h"
#include "../../../Core/Comm/MACommSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDBackendCoordinator, Log, All);

void FMAHUDBackendCoordinator::HandleSimpleCommandSubmitted(
    UWorld* World,
    UMAUIManager* UIManager,
    AMAHUD* HUD,
    const FString& Command) const
{
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    if (!GI)
    {
        return;
    }

    UMACommSubsystem* CommSubsystem = GI->GetSubsystem<UMACommSubsystem>();
    if (CommSubsystem)
    {
        if (HUD && !CommSubsystem->OnPlannerResponse.IsAlreadyBound(HUD, &AMAHUD::OnPlannerResponse))
        {
            CommSubsystem->OnPlannerResponse.AddDynamic(HUD, &AMAHUD::OnPlannerResponse);
        }

        CommSubsystem->SendNaturalLanguageCommand(Command);
        return;
    }

    UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("HandleSimpleCommandSubmitted: CommSubsystem not found"));
    if (UMATaskPlannerWidget* TaskPlannerWidget = UIManager ? UIManager->GetTaskPlannerWidget() : nullptr)
    {
        TaskPlannerWidget->AppendStatusLog(TEXT("[Error] Communication subsystem not found"));
    }
}

void FMAHUDBackendCoordinator::HandlePlannerResponse(UMAUIManager* UIManager, const FMAPlannerResponse& Response) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("HandlePlannerResponse: UIManager is null"));
        return;
    }

    UMATaskPlannerWidget* TaskPlannerWidget = UIManager->GetTaskPlannerWidget();

    if (TaskPlannerWidget)
    {
        TaskPlannerWidget->AppendStatusLog(FString::Printf(
            TEXT("[%s] %s"),
            Response.bSuccess ? TEXT("Success") : TEXT("Failed"),
            *Response.Message));

        if (!Response.PlanText.IsEmpty())
        {
            TaskPlannerWidget->LoadTaskGraphFromJson(Response.PlanText);
        }
        return;
    }
}

void FMAHUDBackendCoordinator::BindBackendEvents(UWorld* World, UMAUIManager* UIManager, AMAHUD* HUD) const
{
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    if (!GI)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("BindBackendEvents: GameInstance not found"));
        return;
    }

    UMACommSubsystem* CommSubsystem = GI->GetSubsystem<UMACommSubsystem>();
    if (CommSubsystem && HUD)
    {
        if (!CommSubsystem->OnTaskPlanReceived.IsAlreadyBound(HUD, &AMAHUD::OnTaskGraphReceived))
        {
            CommSubsystem->OnTaskPlanReceived.AddDynamic(HUD, &AMAHUD::OnTaskGraphReceived);
            UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("BindBackendEvents: Bound OnTaskPlanReceived"));
        }

        if (!CommSubsystem->OnSkillAllocationReceived.IsAlreadyBound(HUD, &AMAHUD::OnSkillAllocationReceived))
        {
            CommSubsystem->OnSkillAllocationReceived.AddDynamic(HUD, &AMAHUD::OnSkillAllocationReceived);
            UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("BindBackendEvents: Bound OnSkillAllocationReceived"));
        }

        if (!CommSubsystem->OnSkillListReceived.IsAlreadyBound(HUD, &AMAHUD::OnSkillListReceived))
        {
            CommSubsystem->OnSkillListReceived.AddDynamic(HUD, &AMAHUD::OnSkillListReceived);
            UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("BindBackendEvents: Bound OnSkillListReceived"));
        }
    }
    else
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("BindBackendEvents: CommSubsystem or HUD not found"));
    }

    BindModalDelegates(UIManager, HUD);
    UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("BindBackendEvents: Backend events bound"));
}

void FMAHUDBackendCoordinator::BindModalDelegates(UMAUIManager* UIManager, AMAHUD* HUD) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("BindModalDelegates: UIManager is null"));
        return;
    }

    if (!HUD)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("BindModalDelegates: HUD is null"));
        return;
    }

    UMAHUDStateManager* StateManager = UIManager->GetHUDStateManager();
    if (!StateManager)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("BindModalDelegates: HUDStateManager is null"));
        return;
    }

    if (!StateManager->OnModalConfirmed.IsAlreadyBound(HUD, &AMAHUD::OnModalConfirmedHandler))
    {
        StateManager->OnModalConfirmed.AddDynamic(HUD, &AMAHUD::OnModalConfirmedHandler);
        UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("BindModalDelegates: Bound OnModalConfirmed"));
    }

    if (!StateManager->OnModalRejected.IsAlreadyBound(HUD, &AMAHUD::OnModalRejectedHandler))
    {
        StateManager->OnModalRejected.AddDynamic(HUD, &AMAHUD::OnModalRejectedHandler);
        UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("BindModalDelegates: Bound OnModalRejected"));
    }

    UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("BindModalDelegates: Modal delegates bound"));
}

void FMAHUDBackendCoordinator::HandleTaskGraphReceived(UMAUIManager* UIManager, const FMATaskPlan& TaskPlan) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("HandleTaskGraphReceived: UIManager is null"));
        return;
    }

    UMAHUDStateManager* StateManager = UIManager->GetHUDStateManager();
    if (StateManager)
    {
        StateManager->ShowNotification(EMANotificationType::TaskGraphUpdate);
        UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleTaskGraphReceived: Notification shown"));
    }
}

void FMAHUDBackendCoordinator::HandleSkillAllocationReceived(UMAUIManager* UIManager, const FMASkillAllocationData& AllocationData) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("HandleSkillAllocationReceived: UIManager is null"));
        return;
    }

    UMAHUDStateManager* StateManager = UIManager->GetHUDStateManager();
    if (StateManager)
    {
        StateManager->ShowNotification(EMANotificationType::SkillListUpdate);
        UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleSkillAllocationReceived: Notification shown, Name=%s"), *AllocationData.Name);
    }
}

void FMAHUDBackendCoordinator::HandleSkillListReceived(UMAUIManager* UIManager, const FMASkillListMessage& SkillList, bool bExecutable) const
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("HandleSkillListReceived: UIManager is null"));
        return;
    }

    UMAHUDStateManager* StateManager = UIManager->GetHUDStateManager();
    if (StateManager)
    {
        StateManager->ShowNotification(
            bExecutable ? EMANotificationType::SkillListExecuting : EMANotificationType::SkillListUpdate);
        UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleSkillListReceived: Notification shown"));
    }
}

void FMAHUDBackendCoordinator::HandleModalConfirmed(UWorld* World, UMAUIManager* UIManager, EMAModalType ModalType) const
{
    UMACommSubsystem* CommSubsystem = GetCommSubsystem(World);
    if (!CommSubsystem || !UIManager)
    {
        return;
    }

    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        {
            UMATaskGraphModal* TaskGraphModal = UIManager->GetTaskGraphModal();
            if (!TaskGraphModal)
            {
                break;
            }

            const FString TaskGraphJson = TaskGraphModal->GetTaskGraphJson();
            if (TaskGraphJson.IsEmpty())
            {
                break;
            }

            CommSubsystem->SendTaskGraphSubmitMessage(TaskGraphJson);
            UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleModalConfirmed: Task graph submitted"));
        }
        break;

    case EMAModalType::SkillAllocation:
        {
            UMASkillAllocationModal* SkillModal = UIManager->GetSkillAllocationModal();
            if (!SkillModal)
            {
                break;
            }

            const FMASkillAllocationData Data = SkillModal->GetSkillAllocationData();
            const FString ModifiedDataJson = Data.ToJson();
            CommSubsystem->SendReviewResponseSimple(true, ModifiedDataJson, TEXT(""));
            UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleModalConfirmed: Skill allocation approved"));
        }
        break;

    default:
        break;
    }
}

bool FMAHUDBackendCoordinator::HandleModalRejected(UWorld* World, UMAUIManager* UIManager, EMAModalType ModalType) const
{
    UMACommSubsystem* CommSubsystem = GetCommSubsystem(World);
    if (!CommSubsystem)
    {
        return false;
    }

    switch (ModalType)
    {
    case EMAModalType::SkillAllocation:
        {
            if (!UIManager)
            {
                break;
            }

            UMASkillAllocationModal* SkillModal = UIManager->GetSkillAllocationModal();
            if (!SkillModal)
            {
                break;
            }

            CommSubsystem->SendReviewResponseSimple(
                false,
                TEXT(""),
                TEXT("User rejected the skill allocation"));
            UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleModalRejected: Skill allocation rejected"));
        }
        break;

    default:
        {
            FString WidgetName;
            switch (ModalType)
            {
            case EMAModalType::TaskGraph:
                WidgetName = TEXT("TaskGraphModal");
                break;
            default:
                WidgetName = TEXT("UnknownModal");
                break;
            }

            CommSubsystem->SendButtonEventMessage(WidgetName, TEXT("reject"), TEXT("Reject"));
            UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleModalRejected: Reject event sent for %s"), *WidgetName);
        }
        break;
    }

    return true;
}

UMACommSubsystem* FMAHUDBackendCoordinator::GetCommSubsystem(UWorld* World) const
{
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    if (!GI)
    {
        return nullptr;
    }

    return GI->GetSubsystem<UMACommSubsystem>();
}
