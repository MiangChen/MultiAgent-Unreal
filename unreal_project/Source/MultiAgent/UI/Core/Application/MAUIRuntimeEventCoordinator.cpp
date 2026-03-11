// MAUIRuntimeEventCoordinator.cpp
// UIManager 运行时事件桥接协调器实现（L1 Application）

#include "MAUIRuntimeEventCoordinator.h"
#include "../MAUIManager.h"
#include "../MAHUDStateManager.h"
#include "../../HUD/MAMainHUDWidget.h"
#include "../../Components/MANotificationWidget.h"
#include "../../Components/MAPreviewPanel.h"
#include "../Modal/MADecisionModal.h"
#include "../../SkillAllocation/MASkillAllocationModal.h"

namespace
{
void ApplyPreviewFeedback(UMAUIManager* UIManager, const FMAUIPreviewFeedback& Feedback)
{
    if (!UIManager)
    {
        return;
    }

    if (UMAPreviewPanel* PreviewPanel = UIManager->GetPreviewPanel())
    {
        if (Feedback.bUpdateTaskGraph)
        {
            PreviewPanel->UpdateTaskGraphPreview(Feedback.TaskGraphData);
        }
        if (Feedback.bUpdateSkillAllocation)
        {
            PreviewPanel->UpdateSkillListPreview(Feedback.SkillAllocationData);
        }
        if (Feedback.bUpdateSkillStatus)
        {
            PreviewPanel->UpdateSkillStatus(Feedback.TimeStep, Feedback.RobotId, Feedback.SkillStatus);
        }
    }
}

void ApplyNotificationFeedback(UMAUIManager* UIManager, const FMAUINotificationFeedback& Feedback)
{
    if (UIManager)
    {
        UIManager->ShowNotification(Feedback.Type);
    }
}

void ApplyDecisionModalFeedback(UMAUIManager* UIManager, const FMAUIDecisionModalFeedback& Feedback)
{
    if (!UIManager)
    {
        return;
    }

    if (UMADecisionModal* DecisionModal = UIManager->GetDecisionModal())
    {
        DecisionModal->LoadDecisionData(Feedback.Description, Feedback.ContextJson);
        UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionDataReceived: Decision data loaded into DecisionModal"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnDecisionDataReceived: DecisionModal is null"));
    }
}
}

void FMAUIRuntimeEventCoordinator::BindTempDataManagerEvents(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    FString RuntimeError;
    if (!UIManager->BindTempDataEventsInternal(RuntimeError))
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindTempDataManagerEvents: %s"), *RuntimeError);
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Bound runtime temp-data events"));

    FMATaskGraphData TaskGraphData;
    if (UIManager->LoadTaskGraphDataInternal(TaskGraphData, RuntimeError) && TaskGraphData.Nodes.Num() > 0)
    {
        HandleTempTaskGraphChanged(UIManager, TaskGraphData);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Loaded existing task graph data (%d nodes)"),
            TaskGraphData.Nodes.Num());
    }

    FMASkillAllocationData SkillAllocationData;
    if (UIManager->LoadSkillAllocationDataInternal(SkillAllocationData, RuntimeError) && SkillAllocationData.Data.Num() > 0)
    {
        HandleTempSkillAllocationChanged(UIManager, SkillAllocationData);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Loaded existing skill allocation data (%d time steps)"),
            SkillAllocationData.Data.Num());
    }
}

void FMAUIRuntimeEventCoordinator::HandleTempTaskGraphChanged(UMAUIManager* UIManager, const FMATaskGraphData& Data) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnTempTaskGraphChanged: Received task graph update (%d nodes, %d edges)"),
        Data.Nodes.Num(), Data.Edges.Num());

    FMAUIPreviewFeedback Feedback;
    Feedback.bUpdateTaskGraph = true;
    Feedback.TaskGraphData = Data;
    ApplyPreviewFeedback(UIManager, Feedback);
}

void FMAUIRuntimeEventCoordinator::HandleTempSkillAllocationChanged(UMAUIManager* UIManager, const FMASkillAllocationData& Data) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnTempSkillAllocationChanged: Received skill allocation update (%d time steps)"),
        Data.Data.Num());

    FMAUIPreviewFeedback Feedback;
    Feedback.bUpdateSkillAllocation = true;
    Feedback.SkillAllocationData = Data;
    ApplyPreviewFeedback(UIManager, Feedback);
}

void FMAUIRuntimeEventCoordinator::HandleSkillStatusUpdated(
    UMAUIManager* UIManager,
    int32 TimeStep,
    const FString& RobotId,
    ESkillExecutionStatus NewStatus) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnSkillStatusUpdated: TimeStep=%d, RobotId=%s, Status=%d"),
        TimeStep, *RobotId, static_cast<int32>(NewStatus));

    FMAUIPreviewFeedback PreviewFeedback;
    PreviewFeedback.bUpdateSkillStatus = true;
    PreviewFeedback.TimeStep = TimeStep;
    PreviewFeedback.RobotId = RobotId;
    PreviewFeedback.SkillStatus = NewStatus;
    ApplyPreviewFeedback(UIManager, PreviewFeedback);

    if (UMASkillAllocationModal* SkillAllocationModal = UIManager->GetSkillAllocationModal())
    {
        SkillAllocationModal->UpdateSkillStatus(TimeStep, RobotId, NewStatus);
    }
}

void FMAUIRuntimeEventCoordinator::BindCommSubsystemEvents(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    FString RuntimeError;
    if (!UIManager->BindCommEventsInternal(RuntimeError))
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindCommSubsystemEvents: %s"), *RuntimeError);
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("BindCommSubsystemEvents: CommSubsystem events bound"));
}

void FMAUIRuntimeEventCoordinator::HandleRequestUserCommandReceived(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnRequestUserCommandReceived: Received request for user command"));
    FMAUINotificationFeedback Feedback;
    Feedback.Type = EMANotificationType::RequestUserCommand;
    ApplyNotificationFeedback(UIManager, Feedback);
}

void FMAUIRuntimeEventCoordinator::BindCommandManagerEvents(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    FString RuntimeError;
    if (!UIManager->BindCommandEventsInternal(RuntimeError))
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindCommandManagerEvents: %s"), *RuntimeError);
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("BindCommandManagerEvents: CommandManager events bound"));
}

void FMAUIRuntimeEventCoordinator::HandleExecutionPauseStateChanged(UMAUIManager* UIManager, bool bPaused) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnExecutionPauseStateChanged: bPaused=%s"), bPaused ? TEXT("true") : TEXT("false"));

    FString RuntimeError;
    UIManager->ClearResumeNotificationTimerInternal(RuntimeError);

    if (bPaused)
    {
        FMAUINotificationFeedback Feedback;
        Feedback.Type = EMANotificationType::SkillListPaused;
        ApplyNotificationFeedback(UIManager, Feedback);
        return;
    }

    FMAUINotificationFeedback Feedback;
    Feedback.Type = EMANotificationType::SkillListResumed;
    ApplyNotificationFeedback(UIManager, Feedback);
    if (!UIManager->ScheduleResumeNotificationAutoHideInternal(2.0f, RuntimeError))
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnExecutionPauseStateChanged: %s"), *RuntimeError);
        return;
    }
}

void FMAUIRuntimeEventCoordinator::HandleDecisionDataReceived(
    UMAUIManager* UIManager,
    const FString& Description,
    const FString& ContextJson) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionDataReceived: Description='%s'"), *Description);

    FMAUIDecisionModalFeedback DecisionFeedback;
    DecisionFeedback.Description = Description;
    DecisionFeedback.ContextJson = ContextJson;
    ApplyDecisionModalFeedback(UIManager, DecisionFeedback);

    FMAUINotificationFeedback NotificationFeedback;
    NotificationFeedback.Type = EMANotificationType::DecisionUpdate;
    ApplyNotificationFeedback(UIManager, NotificationFeedback);
}

void FMAUIRuntimeEventCoordinator::HandleDecisionModalConfirmed(
    UMAUIManager* UIManager,
    const FString& SelectedOption,
    const FString& UserFeedback) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionModalConfirmed: SelectedOption=%s, UserFeedback=%s"),
        *SelectedOption, *UserFeedback);

    FString RuntimeError;
    if (!UIManager->SendDecisionResponseInternal(SelectedOption, UserFeedback, RuntimeError))
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnDecisionModalConfirmed: %s"), *RuntimeError);
    }
    else
    {
        UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionModalConfirmed: Decision response sent to backend"));
    }

    if (UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager())
    {
        HUDStateManager->DismissNotification();
    }
    if (UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget();
        MainHUDWidget && MainHUDWidget->GetNotification())
    {
        MainHUDWidget->GetNotification()->HideNotification();
    }
}

void FMAUIRuntimeEventCoordinator::HandleDecisionModalRejected(
    UMAUIManager* UIManager,
    const FString& SelectedOption,
    const FString& UserFeedback) const
{
    if (!UIManager)
    {
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionModalRejected: SelectedOption=%s, UserFeedback=%s"),
        *SelectedOption, *UserFeedback);

    FString RuntimeError;
    if (!UIManager->SendDecisionResponseInternal(SelectedOption, UserFeedback, RuntimeError))
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnDecisionModalRejected: %s"), *RuntimeError);
    }
    else
    {
        UE_LOG(LogMAUIManager, Log, TEXT("OnDecisionModalRejected: Decision response sent to backend"));
    }

    if (UMAHUDStateManager* HUDStateManager = UIManager->GetHUDStateManager())
    {
        HUDStateManager->DismissNotification();
    }
    if (UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget();
        MainHUDWidget && MainHUDWidget->GetNotification())
    {
        MainHUDWidget->GetNotification()->HideNotification();
    }
}
