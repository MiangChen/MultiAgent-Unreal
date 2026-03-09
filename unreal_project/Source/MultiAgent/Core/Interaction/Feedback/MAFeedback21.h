#pragma once

#include "CoreMinimal.h"

enum class EMAFeedback21MessageSeverity : uint8
{
    Info,
    Success,
    Warning,
    Error
};

enum class EMAFeedback21HUDAction : uint8
{
    None,
    CheckTask,
    CheckSkill,
    CheckDecision,
    ToggleSystemLogPanel,
    TogglePreviewPanel,
    ToggleInstructionPanel,
    ToggleAgentHighlight,
    ShowModifyWidget,
    HideModifyWidget,
    ShowEditWidget,
    HideEditWidget,
    RefreshSceneList,
    ReloadSceneVisualization,
    RefreshSelection,
    ClearHighlightedActor
};

enum class EMAFeedback21NotificationKind : uint8
{
    None,
    TaskGraphUpdate,
    SkillListUpdate,
    SkillListExecuting,
    RequestUserCommand,
    SkillListPaused,
    SkillListResumed,
    DecisionUpdate
};

struct FMAFeedback21Message
{
    FString Text;
    EMAFeedback21MessageSeverity Severity = EMAFeedback21MessageSeverity::Info;
    float Duration = 2.f;
};

struct FMAFeedback21Batch
{
    TArray<EMAFeedback21HUDAction> HUDActions;
    TArray<EMAFeedback21NotificationKind> Notifications;
    TArray<FMAFeedback21Message> Messages;
    TArray<FString> TaskPlannerLogs;
    FString TaskGraphJson;
    bool bReplaceTaskPlannerGraph = false;

    bool bHasSelectionBoxSync = false;
    bool bSelectionBoxVisible = false;
    FVector2D SelectionBoxStart = FVector2D::ZeroVector;
    FVector2D SelectionBoxEnd = FVector2D::ZeroVector;

    bool IsEmpty() const
    {
        return HUDActions.IsEmpty()
            && Notifications.IsEmpty()
            && Messages.IsEmpty()
            && TaskPlannerLogs.IsEmpty()
            && TaskGraphJson.IsEmpty()
            && !bReplaceTaskPlannerGraph
            && !bHasSelectionBoxSync;
    }

    void AddHUDAction(EMAFeedback21HUDAction Action)
    {
        if (Action != EMAFeedback21HUDAction::None)
        {
            HUDActions.Add(Action);
        }
    }

    void AddNotification(EMAFeedback21NotificationKind Kind)
    {
        if (Kind != EMAFeedback21NotificationKind::None)
        {
            Notifications.Add(Kind);
        }
    }

    void AddMessage(
        const FString& Text,
        EMAFeedback21MessageSeverity Severity = EMAFeedback21MessageSeverity::Info,
        float Duration = 2.f)
    {
        if (!Text.IsEmpty())
        {
            Messages.Add({Text, Severity, Duration});
        }
    }

    void AddTaskPlannerLog(const FString& Line)
    {
        if (!Line.IsEmpty())
        {
            TaskPlannerLogs.Add(Line);
        }
    }

    void SetTaskPlannerGraph(const FString& Json)
    {
        TaskGraphJson = Json;
        bReplaceTaskPlannerGraph = !Json.IsEmpty();
    }

    void SetSelectionBox(bool bVisible, const FVector2D& Start, const FVector2D& End)
    {
        bHasSelectionBoxSync = true;
        bSelectionBoxVisible = bVisible;
        SelectionBoxStart = Start;
        SelectionBoxEnd = End;
    }

    void Append(const FMAFeedback21Batch& Other)
    {
        HUDActions.Append(Other.HUDActions);
        Notifications.Append(Other.Notifications);
        Messages.Append(Other.Messages);
        TaskPlannerLogs.Append(Other.TaskPlannerLogs);

        if (Other.bReplaceTaskPlannerGraph)
        {
            TaskGraphJson = Other.TaskGraphJson;
            bReplaceTaskPlannerGraph = true;
        }

        if (Other.bHasSelectionBoxSync)
        {
            bHasSelectionBoxSync = true;
            bSelectionBoxVisible = Other.bSelectionBoxVisible;
            SelectionBoxStart = Other.SelectionBoxStart;
            SelectionBoxEnd = Other.SelectionBoxEnd;
        }
    }
};
