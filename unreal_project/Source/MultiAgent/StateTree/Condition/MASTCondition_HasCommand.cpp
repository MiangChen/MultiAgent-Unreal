// MASTCondition_HasCommand.cpp

#include "MASTCondition_HasCommand.h"
#include "../../Character/MACharacter.h"
#include "../../GAS/MAAbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"

bool FMASTCondition_HasCommand::TestCondition(FStateTreeExecutionContext& Context) const
{
    if (!CommandTag.IsValid())
    {
        return false;
    }

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner)
    {
        return false;
    }

    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character)
    {
        return false;
    }

    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
    if (!ASC)
    {
        return false;
    }

    return ASC->HasGameplayTagFromContainer(CommandTag);
}
