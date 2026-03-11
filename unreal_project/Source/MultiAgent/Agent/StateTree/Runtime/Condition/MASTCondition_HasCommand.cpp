// MASTCondition_HasCommand.cpp

#include "MASTCondition_HasCommand.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

bool FMASTCondition_HasCommand::TestCondition(FStateTreeExecutionContext& Context) const
{
    if (!CommandTag.IsValid()) return false;

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner) return false;

    UMASkillComponent* SkillComp = Owner->FindComponentByClass<UMASkillComponent>();
    if (!SkillComp) return false;

    return SkillComp->HasMatchingGameplayTag(CommandTag);
}
