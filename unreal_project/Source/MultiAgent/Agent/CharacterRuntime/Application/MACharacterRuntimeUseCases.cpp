#include "Agent/CharacterRuntime/Application/MACharacterRuntimeUseCases.h"

FMACharacterDirectControlFeedback FMACharacterRuntimeUseCases::BuildDirectControlTransition(
    const bool bIsUnderDirectControl,
    const bool bEnable)
{
    FMACharacterDirectControlFeedback Feedback;

    if (bIsUnderDirectControl == bEnable)
    {
        return Feedback;
    }

    Feedback.Transition = bEnable
        ? EMACharacterDirectControlTransition::Enable
        : EMACharacterDirectControlTransition::Disable;
    Feedback.bShouldCancelAIMovement = bEnable;
    Feedback.bOrientRotationToMovement = !bEnable;
    return Feedback;
}

FMACharacterLowEnergyTriggerFeedback FMACharacterRuntimeUseCases::BuildLowEnergyTrigger(const bool bIsExecutionPaused)
{
    FMACharacterLowEnergyTriggerFeedback Feedback;
    Feedback.bShouldCancelSkills = bIsExecutionPaused;
    Feedback.StatusMessage = TEXT("[Low Battery] Returning to base...");
    Feedback.StatusDuration = 5.f;

    if (bIsExecutionPaused)
    {
        Feedback.Action = EMACharacterLowEnergyAction::DeferUntilResume;
        Feedback.bShouldBindPauseStateChanged = true;
        Feedback.LogMessage = TEXT("Paused, deferring low-energy return until resume");
        return Feedback;
    }

    Feedback.Action = EMACharacterLowEnergyAction::ExecuteReturn;
    return Feedback;
}

FMACharacterLowEnergyTriggerFeedback FMACharacterRuntimeUseCases::BuildPauseResumeReaction(
    const bool bPaused,
    const bool bPendingLowEnergyReturn)
{
    FMACharacterLowEnergyTriggerFeedback Feedback;
    if (bPaused || !bPendingLowEnergyReturn)
    {
        return Feedback;
    }

    Feedback.Action = EMACharacterLowEnergyAction::ExecuteReturn;
    Feedback.bShouldUnbindPauseStateChanged = true;
    Feedback.bShouldClearPendingReturn = true;
    return Feedback;
}

FMACharacterRuntimeFeedback FMACharacterRuntimeUseCases::BuildLowEnergyReturnPlan(const bool bIsFlying)
{
    FMACharacterRuntimeFeedback Feedback;
    Feedback.bSuccess = true;

    const bool bUseReturnHome = bIsFlying;
    Feedback.ReturnMode = bUseReturnHome ? EMACharacterReturnMode::ReturnHome : EMACharacterReturnMode::NavigateHome;
    Feedback.Severity = EMACharacterRuntimeSeverity::Warning;
    Feedback.Message = bUseReturnHome
        ? TEXT("Low battery, starting ReturnHome")
        : TEXT("Low battery, navigating back to initial position");
    return Feedback;
}
