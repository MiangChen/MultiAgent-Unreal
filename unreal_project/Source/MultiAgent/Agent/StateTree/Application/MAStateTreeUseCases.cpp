#include "Agent/StateTree/Application/MAStateTreeUseCases.h"

FMAStateTreeLifecycleFeedback FMAStateTreeUseCases::BuildBeginPlayFeedback(const bool bStateTreeEnabled, const bool bHasAsset)
{
    FMAStateTreeLifecycleFeedback Feedback;

    if (!bStateTreeEnabled)
    {
        Feedback.Mode = EMAStateTreeLifecycleMode::Disabled;
        Feedback.Message = TEXT("StateTree disabled by config");
        return Feedback;
    }

    if (!bHasAsset)
    {
        Feedback.Mode = EMAStateTreeLifecycleMode::WaitingForAsset;
        Feedback.Message = TEXT("StateTree enabled, waiting for asset injection");
        return Feedback;
    }

    Feedback.Mode = EMAStateTreeLifecycleMode::Ready;
    Feedback.bShouldStartLogic = true;
    Feedback.Message = TEXT("StateTree ready");
    return Feedback;
}

FMAStateTreeLifecycleFeedback FMAStateTreeUseCases::BuildEnableFeedback(const bool bHasAsset)
{
    FMAStateTreeLifecycleFeedback Feedback;
    Feedback.Mode = bHasAsset ? EMAStateTreeLifecycleMode::Ready : EMAStateTreeLifecycleMode::WaitingForAsset;
    Feedback.bShouldStartLogic = bHasAsset;
    Feedback.Message = bHasAsset
        ? TEXT("StateTree enabled and ready to start")
        : TEXT("StateTree enabled, waiting for asset");
    return Feedback;
}

EMAStateTreeTaskDecision FMAStateTreeUseCases::BuildCommandEnterDecision(const bool bActivationSucceeded)
{
    return bActivationSucceeded
        ? EMAStateTreeTaskDecision::Running
        : EMAStateTreeTaskDecision::Failed;
}

EMAStateTreeTaskDecision FMAStateTreeUseCases::BuildCommandTickDecision(
    const bool bHasSkillComponent,
    const bool bCommandCompleted)
{
    if (!bHasSkillComponent)
    {
        return EMAStateTreeTaskDecision::Failed;
    }

    return bCommandCompleted
        ? EMAStateTreeTaskDecision::Succeeded
        : EMAStateTreeTaskDecision::Running;
}

EMAStateTreeTaskDecision FMAStateTreeUseCases::BuildFollowTickDecision(
    const bool bHasSkillComponent,
    const bool bCommandCompleted,
    const bool bHasValidTarget)
{
    if (!bHasSkillComponent || !bHasValidTarget)
    {
        return EMAStateTreeTaskDecision::Failed;
    }

    return bCommandCompleted
        ? EMAStateTreeTaskDecision::Succeeded
        : EMAStateTreeTaskDecision::Running;
}

EMAStateTreeTaskDecision FMAStateTreeUseCases::BuildPlaceEnterDecision(
    const bool bHasSearchTarget,
    const bool bActivationSucceeded)
{
    if (!bHasSearchTarget)
    {
        return EMAStateTreeTaskDecision::Failed;
    }

    return BuildCommandEnterDecision(bActivationSucceeded);
}

FMAStateTreeTaskExitFeedback FMAStateTreeUseCases::BuildInterruptedCommandExit(const bool bWasRunning)
{
    FMAStateTreeTaskExitFeedback Feedback;
    Feedback.bShouldCancelCommand = bWasRunning;
    return Feedback;
}

FMAStateTreeTaskExitFeedback FMAStateTreeUseCases::BuildActivatedCommandExit(
    const bool bSkillActivated,
    const bool bTransitionCommandToIdle)
{
    FMAStateTreeTaskExitFeedback Feedback;
    Feedback.bShouldCancelCommand = bSkillActivated;
    Feedback.bShouldTransitionCommandToIdle = bSkillActivated && bTransitionCommandToIdle;
    return Feedback;
}

FMAStateTreeChargeExitFeedback FMAStateTreeUseCases::BuildChargeExitFeedback(
    const bool bIsMoving,
    const bool bIsCharging)
{
    FMAStateTreeChargeExitFeedback Feedback;
    Feedback.bShouldCancelNavigate = bIsMoving;
    Feedback.bShouldCancelCharge = bIsCharging;
    return Feedback;
}
