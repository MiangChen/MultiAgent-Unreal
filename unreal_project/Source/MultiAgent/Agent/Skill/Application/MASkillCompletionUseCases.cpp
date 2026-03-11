#include "Agent/Skill/Application/MASkillCompletionUseCases.h"

#include "Agent/Skill/Infrastructure/MAFeedbackGenerator.h"
#include "Agent/Skill/Infrastructure/MASceneGraphUpdater.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"

namespace
{
FGameplayTag CompletionCommandToTag(const EMACommand Command)
{
    switch (Command)
    {
        case EMACommand::Idle: return FGameplayTag::RequestGameplayTag(FName("Command.Idle"));
        case EMACommand::Navigate: return FGameplayTag::RequestGameplayTag(FName("Command.Navigate"));
        case EMACommand::Follow: return FGameplayTag::RequestGameplayTag(FName("Command.Follow"));
        case EMACommand::Charge: return FGameplayTag::RequestGameplayTag(FName("Command.Charge"));
        case EMACommand::Search: return FGameplayTag::RequestGameplayTag(FName("Command.Search"));
        case EMACommand::Place: return FGameplayTag::RequestGameplayTag(FName("Command.Place"));
        case EMACommand::TakeOff: return FGameplayTag::RequestGameplayTag(FName("Command.TakeOff"));
        case EMACommand::Land: return FGameplayTag::RequestGameplayTag(FName("Command.Land"));
        case EMACommand::ReturnHome: return FGameplayTag::RequestGameplayTag(FName("Command.ReturnHome"));
        case EMACommand::TakePhoto: return FGameplayTag::RequestGameplayTag(FName("Command.TakePhoto"));
        case EMACommand::Broadcast: return FGameplayTag::RequestGameplayTag(FName("Command.Broadcast"));
        case EMACommand::HandleHazard: return FGameplayTag::RequestGameplayTag(FName("Command.HandleHazard"));
        case EMACommand::Guide: return FGameplayTag::RequestGameplayTag(FName("Command.Guide"));
        case EMACommand::None:
        default:
            return FGameplayTag();
    }
}
}

bool FMASkillCompletionUseCases::NotifyAbilityFinished(
    UMASkillComponent& SkillComponent,
    const EMACommand Command,
    const bool bSuccess,
    const FString& Message,
    const bool bSwitchToIdle)
{
    const FGameplayTag CommandTag = CompletionCommandToTag(Command);
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
        SkillComponent.AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
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
