#pragma once

#include "CoreMinimal.h"
#include "Agent/StateTree/Feedback/MAStateTreeFeedback.h"

struct MULTIAGENT_API FMAStateTreeUseCases
{
    static FMAStateTreeLifecycleFeedback BuildBeginPlayFeedback(bool bStateTreeEnabled, bool bHasAsset);
    static FMAStateTreeLifecycleFeedback BuildEnableFeedback(bool bHasAsset);
    static EMAStateTreeTaskDecision BuildCommandEnterDecision(bool bActivationSucceeded);
    static EMAStateTreeTaskDecision BuildCommandTickDecision(bool bHasSkillComponent, bool bCommandCompleted);
    static EMAStateTreeTaskDecision BuildFollowTickDecision(bool bHasSkillComponent, bool bCommandCompleted, bool bHasValidTarget);
    static EMAStateTreeTaskDecision BuildPlaceEnterDecision(bool bHasSearchTarget, bool bActivationSucceeded);
    static FMAStateTreeTaskExitFeedback BuildInterruptedCommandExit(bool bWasRunning);
    static FMAStateTreeTaskExitFeedback BuildActivatedCommandExit(bool bSkillActivated, bool bTransitionCommandToIdle);
    static FMAStateTreeChargeExitFeedback BuildChargeExitFeedback(bool bIsMoving, bool bIsCharging);
};
