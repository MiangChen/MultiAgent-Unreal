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
