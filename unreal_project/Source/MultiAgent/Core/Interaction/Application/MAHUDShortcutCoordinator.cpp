// HUD shortcut coordination for player input.

#include "MAHUDShortcutCoordinator.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDShortcutCoordinator, Log, All);

FMAFeedback21Batch FMAHUDShortcutCoordinator::HandleCheckTask() const
{
    FMAFeedback21Batch Feedback;
    Feedback.AddHUDAction(EMAFeedback21HUDAction::CheckTask);
    return Feedback;
}

FMAFeedback21Batch FMAHUDShortcutCoordinator::HandleCheckSkill() const
{
    FMAFeedback21Batch Feedback;
    Feedback.AddHUDAction(EMAFeedback21HUDAction::CheckSkill);
    return Feedback;
}

FMAFeedback21Batch FMAHUDShortcutCoordinator::HandleCheckDecision() const
{
    FMAFeedback21Batch Feedback;
    Feedback.AddHUDAction(EMAFeedback21HUDAction::CheckDecision);
    return Feedback;
}

FMAFeedback21Batch FMAHUDShortcutCoordinator::ToggleSystemLogPanel() const
{
    FMAFeedback21Batch Feedback;
    Feedback.AddHUDAction(EMAFeedback21HUDAction::ToggleSystemLogPanel);
    return Feedback;
}

FMAFeedback21Batch FMAHUDShortcutCoordinator::TogglePreviewPanel() const
{
    FMAFeedback21Batch Feedback;
    Feedback.AddHUDAction(EMAFeedback21HUDAction::TogglePreviewPanel);
    return Feedback;
}

FMAFeedback21Batch FMAHUDShortcutCoordinator::ToggleInstructionPanel() const
{
    FMAFeedback21Batch Feedback;
    Feedback.AddHUDAction(EMAFeedback21HUDAction::ToggleInstructionPanel);
    return Feedback;
}

FMAFeedback21Batch FMAHUDShortcutCoordinator::ToggleAgentHighlight() const
{
    FMAFeedback21Batch Feedback;
    Feedback.AddHUDAction(EMAFeedback21HUDAction::ToggleAgentHighlight);
    return Feedback;
}
