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

    // 使用 HasMatchingGameplayTag 与 Task 中的检查保持一致
    bool bHasTag = ASC->HasMatchingGameplayTag(CommandTag);
    
    // 调试日志（可选，帮助排查问题）
    // UE_LOG(LogTemp, Verbose, TEXT("[HasCommand] %s checking %s = %d"), 
    //     *Character->ActorName, *CommandTag.ToString(), bHasTag ? 1 : 0);
    
    return bHasTag;
}
