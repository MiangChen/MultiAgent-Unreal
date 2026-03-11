#include "Agent/Skill/Application/MASkillCompletionUseCases.h"

#include "Agent/Skill/Infrastructure/MAFeedbackGenerator.h"
#include "Agent/Skill/Infrastructure/MASceneGraphUpdater.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Core/Command/Domain/MACommandTags.h"

bool FMASkillCompletionUseCases::NotifyAbilityFinished(
    UMASkillComponent& SkillComponent,
    const EMACommand Command,
    const bool bSuccess,
    const FString& Message,
    const bool bSwitchToIdle)
{
    const FGameplayTag CommandTag = FMACommandTags::ToTag(Command);
    if (CommandTag.IsValid())
    {
        if (!SkillComponent.HasMatchingGameplayTag(CommandTag))
        {
            return false;
        }
        SkillComponent.RemoveLooseGameplayTag(CommandTag);
    }

    if (bSwitchToIdle)
    {
        SkillComponent.AddLooseGameplayTag(FMACommandTags::ToTag(EMACommand::Idle));
    }

    SkillComponent.NotifySkillCompleted(bSuccess, Message);
    return true;
}

FMASkillExecutionFeedback FMASkillCompletionUseCases::BuildCompletionFeedback(
    AMACharacter& Agent,
    const EMACommand Command,
    const bool bSuccess,
    const FString& Message)
{
    FMASceneGraphUpdater::UpdateAfterSkillCompletion(&Agent, Command, bSuccess);
    return FMAFeedbackGenerator::Generate(&Agent, Command, bSuccess, Message);
}
