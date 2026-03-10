// Applies L2 -> L1 UI feedback to player-controller and HUD entry points.

#include "MAFeedback21Applier.h"

#include "../../../UI/HUD/MAHUD.h"
#include "../../../UI/Core/MAUIManager.h"
#include "../../../UI/Core/MAHUDStateManager.h"
#include "../../../UI/Core/MAHUDTypes.h"
#include "../../../UI/SceneEditing/MASceneListWidget.h"
#include "../../../UI/TaskGraph/MATaskPlannerWidget.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

namespace
{
EMANotificationType MapNotificationKind(const EMAFeedback21NotificationKind Kind)
{
    switch (Kind)
    {
    case EMAFeedback21NotificationKind::TaskGraphUpdate:
        return EMANotificationType::TaskGraphUpdate;
    case EMAFeedback21NotificationKind::SkillListUpdate:
        return EMANotificationType::SkillListUpdate;
    case EMAFeedback21NotificationKind::SkillListExecuting:
        return EMANotificationType::SkillListExecuting;
    case EMAFeedback21NotificationKind::RequestUserCommand:
        return EMANotificationType::RequestUserCommand;
    case EMAFeedback21NotificationKind::SkillListPaused:
        return EMANotificationType::SkillListPaused;
    case EMAFeedback21NotificationKind::SkillListResumed:
        return EMANotificationType::SkillListResumed;
    case EMAFeedback21NotificationKind::DecisionUpdate:
        return EMANotificationType::DecisionUpdate;
    case EMAFeedback21NotificationKind::None:
    default:
        return EMANotificationType::None;
    }
}

FColor MapSeverityColor(const EMAFeedback21MessageSeverity Severity)
{
    switch (Severity)
    {
    case EMAFeedback21MessageSeverity::Success:
        return FColor::Green;
    case EMAFeedback21MessageSeverity::Warning:
        return FColor::Yellow;
    case EMAFeedback21MessageSeverity::Error:
        return FColor::Red;
    case EMAFeedback21MessageSeverity::Info:
    default:
        return FColor::Cyan;
    }
}
}

void FMAFeedback21Applier::ApplyToPlayerController(
    APlayerController* PlayerController,
    const FMAFeedback21Batch& Feedback) const
{
    if (!PlayerController || Feedback.IsEmpty())
    {
        return;
    }

    for (const EMAFeedback21HUDAction Action : Feedback.HUDActions)
    {
        ApplyHUDActionToPlayerController(PlayerController, Action);
    }

    if (Feedback.bHasSelectionBoxSync)
    {
        HUDInputAdapter.SyncSelectionBox(
            PlayerController,
            Feedback.bSelectionBoxVisible,
            Feedback.SelectionBoxStart,
            Feedback.SelectionBoxEnd);
    }

    for (const EMAFeedback21NotificationKind Kind : Feedback.Notifications)
    {
        if (UMAHUDStateManager* StateManager = HUDInputAdapter.ResolveHUDStateManager(PlayerController))
        {
            const EMANotificationType NotificationType = MapNotificationKind(Kind);
            if (NotificationType != EMANotificationType::None)
            {
                StateManager->ShowNotification(NotificationType);
            }
        }
    }

    for (const FMAFeedback21Message& Message : Feedback.Messages)
    {
        ShowPlayerMessage(Message);
    }

    if (Feedback.bReplaceTaskPlannerGraph || !Feedback.TaskPlannerLogs.IsEmpty())
    {
        if (AMAHUD* HUD = HUDInputAdapter.ResolveHUD(PlayerController))
        {
            ApplyToHUD(HUD, Feedback);
        }
    }
}

void FMAFeedback21Applier::ApplyToHUD(AMAHUD* HUD, const FMAFeedback21Batch& Feedback) const
{
    if (!HUD || Feedback.IsEmpty())
    {
        return;
    }

    for (const EMAFeedback21HUDAction Action : Feedback.HUDActions)
    {
        ApplyHUDActionToHUD(HUD, Action);
    }

    for (const EMAFeedback21NotificationKind Kind : Feedback.Notifications)
    {
        if (UMAHUDStateManager* StateManager = HUD->GetHUDStateManager())
        {
            const EMANotificationType NotificationType = MapNotificationKind(Kind);
            if (NotificationType != EMANotificationType::None)
            {
                StateManager->ShowNotification(NotificationType);
            }
        }
    }

    for (const FMAFeedback21Message& Message : Feedback.Messages)
    {
        HUD->ShowNotification(
            Message.Text,
            Message.Severity == EMAFeedback21MessageSeverity::Error,
            Message.Severity == EMAFeedback21MessageSeverity::Warning);
    }

    if (UMATaskPlannerWidget* TaskPlannerWidget = HUD->GetTaskPlannerWidget())
    {
        for (const FString& Line : Feedback.TaskPlannerLogs)
        {
            TaskPlannerWidget->AppendStatusLog(Line);
        }

        if (Feedback.bReplaceTaskPlannerGraph)
        {
            TaskPlannerWidget->LoadTaskGraphFromJson(Feedback.TaskGraphJson);
        }
    }
}

void FMAFeedback21Applier::ApplyHUDActionToPlayerController(
    APlayerController* PlayerController,
    const EMAFeedback21HUDAction Action) const
{
    switch (Action)
    {
    case EMAFeedback21HUDAction::CheckTask:
        if (UMAHUDStateManager* StateManager = HUDInputAdapter.ResolveHUDStateManager(PlayerController))
        {
            StateManager->HandleCheckTaskInput();
        }
        break;
    case EMAFeedback21HUDAction::CheckSkill:
        if (UMAHUDStateManager* StateManager = HUDInputAdapter.ResolveHUDStateManager(PlayerController))
        {
            StateManager->HandleCheckSkillInput();
        }
        break;
    case EMAFeedback21HUDAction::CheckDecision:
        if (UMAHUDStateManager* StateManager = HUDInputAdapter.ResolveHUDStateManager(PlayerController))
        {
            StateManager->HandleCheckDecisionInput();
        }
        break;
    case EMAFeedback21HUDAction::ToggleSystemLogPanel:
        HUDInputAdapter.ToggleSystemLogPanel(PlayerController);
        break;
    case EMAFeedback21HUDAction::TogglePreviewPanel:
        HUDInputAdapter.TogglePreviewPanel(PlayerController);
        break;
    case EMAFeedback21HUDAction::ToggleInstructionPanel:
        HUDInputAdapter.ToggleInstructionPanel(PlayerController);
        break;
    case EMAFeedback21HUDAction::ToggleAgentHighlight:
        ShowPlayerMessage({
            FString::Printf(
                TEXT("Agent Highlight: %s"),
                HUDInputAdapter.ToggleAgentHighlight(PlayerController) ? TEXT("ON") : TEXT("OFF")),
            EMAFeedback21MessageSeverity::Info,
            2.f});
        break;
    case EMAFeedback21HUDAction::ShowModifyWidget:
        HUDInputAdapter.ShowModifyWidget(PlayerController);
        break;
    case EMAFeedback21HUDAction::HideModifyWidget:
        HUDInputAdapter.HideModifyWidget(PlayerController);
        break;
    case EMAFeedback21HUDAction::ShowEditWidget:
        HUDInputAdapter.ShowEditWidget(PlayerController);
        break;
    case EMAFeedback21HUDAction::HideEditWidget:
        HUDInputAdapter.HideEditWidget(PlayerController);
        break;
    case EMAFeedback21HUDAction::RefreshSceneList:
    case EMAFeedback21HUDAction::ReloadSceneVisualization:
    case EMAFeedback21HUDAction::RefreshSelection:
    case EMAFeedback21HUDAction::ClearHighlightedActor:
    case EMAFeedback21HUDAction::None:
    default:
        break;
    }
}

void FMAFeedback21Applier::ApplyHUDActionToHUD(AMAHUD* HUD, const EMAFeedback21HUDAction Action) const
{
    switch (Action)
    {
    case EMAFeedback21HUDAction::RefreshSceneList:
        if (HUD->UIManager)
        {
            if (UMASceneListWidget* SceneListWidget = HUD->UIManager->GetSceneListWidget())
            {
                SceneListWidget->RefreshLists();
            }
        }
        break;
    case EMAFeedback21HUDAction::ReloadSceneVisualization:
        HUD->LoadSceneGraphForVisualization();
        break;
    case EMAFeedback21HUDAction::RefreshSelection:
        HUD->OnTempSceneGraphChanged();
        break;
    case EMAFeedback21HUDAction::ClearHighlightedActor:
        HUD->ClearHighlightedActor();
        break;
    case EMAFeedback21HUDAction::CheckTask:
    case EMAFeedback21HUDAction::CheckSkill:
    case EMAFeedback21HUDAction::CheckDecision:
    case EMAFeedback21HUDAction::ToggleSystemLogPanel:
    case EMAFeedback21HUDAction::TogglePreviewPanel:
    case EMAFeedback21HUDAction::ToggleInstructionPanel:
    case EMAFeedback21HUDAction::ToggleAgentHighlight:
    case EMAFeedback21HUDAction::ShowModifyWidget:
    case EMAFeedback21HUDAction::HideModifyWidget:
    case EMAFeedback21HUDAction::ShowEditWidget:
    case EMAFeedback21HUDAction::HideEditWidget:
    case EMAFeedback21HUDAction::None:
    default:
        break;
    }
}

void FMAFeedback21Applier::ShowPlayerMessage(const FMAFeedback21Message& Message) const
{
    if (!Message.Text.IsEmpty() && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, Message.Duration, MapSeverityColor(Message.Severity), Message.Text);
    }
}
