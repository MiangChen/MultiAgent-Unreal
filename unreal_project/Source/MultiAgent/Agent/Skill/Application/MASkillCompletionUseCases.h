#pragma once

#include "CoreMinimal.h"
#include "Core/Command/Domain/MACommandTypes.h"
#include "Agent/Skill/Feedback/MASkillFeedbackTypes.h"

class AMACharacter;
class UMASkillComponent;

struct MULTIAGENT_API FMASkillCompletionUseCases
{
    static bool NotifyAbilityFinished(UMASkillComponent& SkillComponent, EMACommand Command, bool bSuccess, const FString& Message, bool bSwitchToIdle = false);
    static FMASkillExecutionFeedback BuildCompletionFeedback(AMACharacter& Agent, EMACommand Command, bool bSuccess, const FString& Message);
};
