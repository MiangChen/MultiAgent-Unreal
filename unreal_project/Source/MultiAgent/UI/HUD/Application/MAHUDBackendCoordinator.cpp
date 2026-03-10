// MAHUDBackendCoordinator.cpp
// HUD backend interaction coordination.

#include "MAHUDBackendCoordinator.h"
#include "../MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../Core/MAHUDStateManager.h"
#include "../../TaskGraph/MATaskPlannerWidget.h"
#include "../../TaskGraph/MATaskGraphModal.h"
#include "../../SkillAllocation/MASkillAllocationModal.h"
#include "../../../Core/SkillAllocation/Application/MASkillAllocationUseCases.h"
#include "../../../Core/Shared/Types/MASimTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDBackendCoordinator, Log, All);

namespace
{
FMAFeedback21Batch BuildPlannerResponseFeedback(const FMAPlannerResponse& Response)
{
    FMAFeedback21Batch Feedback;
    Feedback.AddTaskPlannerLog(FString::Printf(
        TEXT("[%s] %s"),
        Response.bSuccess ? TEXT("Success") : TEXT("Failed"),
        *Response.Message));

    if (!Response.PlanText.IsEmpty())
    {
        Feedback.SetTaskPlannerGraph(Response.PlanText);
    }

    return Feedback;
}
}

void FMAHUDBackendCoordinator::HandleSimpleCommandSubmitted(
    UWorld* World,
    UMAUIManager* UIManager,
    AMAHUD* HUD,
    const FString& Command) const
{
    if (HUD && HUD->RuntimeSendNaturalLanguageCommand(Command))
    {
        return;
    }

    UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("HandleSimpleCommandSubmitted: CommSubsystem not found"));
    if (HUD)
    {
        FMAFeedback21Batch Feedback;
        Feedback.AddTaskPlannerLog(TEXT("[Error] Communication subsystem not found"));
        HUD->ApplyInteractionFeedback(Feedback);
    }
}

void FMAHUDBackendCoordinator::HandlePlannerResponse(AMAHUD* HUD, UMAUIManager* UIManager, const FMAPlannerResponse& Response) const
{
    if (!HUD || !UIManager)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("HandlePlannerResponse: HUD or UIManager is null"));
        return;
    }

    HUD->ApplyInteractionFeedback(BuildPlannerResponseFeedback(Response));
}

void FMAHUDBackendCoordinator::BindBackendEvents(UWorld* World, UMAUIManager* UIManager, AMAHUD* HUD) const
{
    if (HUD && HUD->RuntimeBindBackendEvents())
    {
        UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("BindBackendEvents: Bound backend delegates"));
    }
    else
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("BindBackendEvents: runtime binding unavailable"));
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

void FMAHUDBackendCoordinator::HandleTaskGraphReceived(AMAHUD* HUD, UMAUIManager* UIManager, const FMATaskPlan& TaskPlan) const
{
    if (!HUD || !UIManager)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("HandleTaskGraphReceived: HUD or UIManager is null"));
        return;
    }

    FMAFeedback21Batch Feedback;
    Feedback.AddNotification(EMAFeedback21NotificationKind::TaskGraphUpdate);
    HUD->ApplyInteractionFeedback(Feedback);
    UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleTaskGraphReceived: Notification shown"));
}

void FMAHUDBackendCoordinator::HandleSkillAllocationReceived(AMAHUD* HUD, UMAUIManager* UIManager, const FMASkillAllocationData& AllocationData) const
{
    if (!HUD || !UIManager)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("HandleSkillAllocationReceived: HUD or UIManager is null"));
        return;
    }

    FMAFeedback21Batch Feedback;
    Feedback.AddNotification(EMAFeedback21NotificationKind::SkillListUpdate);
    HUD->ApplyInteractionFeedback(Feedback);
    UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleSkillAllocationReceived: Notification shown, Name=%s"), *AllocationData.Name);
}

void FMAHUDBackendCoordinator::HandleSkillListReceived(AMAHUD* HUD, UMAUIManager* UIManager, const FMASkillListMessage& SkillList, bool bExecutable) const
{
    if (!HUD || !UIManager)
    {
        UE_LOG(LogMAHUDBackendCoordinator, Warning, TEXT("HandleSkillListReceived: HUD or UIManager is null"));
        return;
    }

    FMAFeedback21Batch Feedback;
    Feedback.AddNotification(
        bExecutable ? EMAFeedback21NotificationKind::SkillListExecuting : EMAFeedback21NotificationKind::SkillListUpdate);
    HUD->ApplyInteractionFeedback(Feedback);
    UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleSkillListReceived: Notification shown"));
}

void FMAHUDBackendCoordinator::HandleModalConfirmed(AMAHUD* HUD, UMAUIManager* UIManager, EMAModalType ModalType) const
{
    if (!UIManager)
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

            HUD->RuntimeSendTaskGraphSubmit(TaskGraphJson);
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
            const FString ModifiedDataJson = FMASkillAllocationUseCases::SerializeJson(Data);
            HUD->RuntimeSendReviewResponse(true, ModifiedDataJson, TEXT(""));
            UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleModalConfirmed: Skill allocation approved"));
        }
        break;

    default:
        break;
    }
}

bool FMAHUDBackendCoordinator::HandleModalRejected(AMAHUD* HUD, UMAUIManager* UIManager, EMAModalType ModalType) const
{
    if (!HUD || !HUD->RuntimeHasCommSubsystem())
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

            HUD->RuntimeSendReviewResponse(false, TEXT(""), TEXT("User rejected the skill allocation"));
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

            HUD->RuntimeSendButtonEvent(WidgetName, TEXT("reject"), TEXT("Reject"));
            UE_LOG(LogMAHUDBackendCoordinator, Log, TEXT("HandleModalRejected: Reject event sent for %s"), *WidgetName);
        }
        break;
    }

    return true;
}
