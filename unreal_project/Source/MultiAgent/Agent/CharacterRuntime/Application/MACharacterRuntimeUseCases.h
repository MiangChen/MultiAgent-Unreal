#pragma once

#include "CoreMinimal.h"
#include "Agent/CharacterRuntime/Feedback/MACharacterRuntimeFeedback.h"

struct MULTIAGENT_API FMACharacterRuntimeUseCases
{
    static FMACharacterDirectControlFeedback BuildDirectControlTransition(bool bIsUnderDirectControl, bool bEnable);
    static FMACharacterLowEnergyTriggerFeedback BuildLowEnergyTrigger(bool bIsExecutionPaused);
    static FMACharacterLowEnergyTriggerFeedback BuildPauseResumeReaction(bool bPaused, bool bPendingLowEnergyReturn);
    static FMACharacterRuntimeFeedback BuildLowEnergyReturnPlan(bool bIsFlying);
};
