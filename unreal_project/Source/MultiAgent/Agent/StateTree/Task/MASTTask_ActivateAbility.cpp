// MASTTask_ActivateAbility.cpp
// 通用技能激活任务

#include "MASTTask_ActivateAbility.h"
#include "../../Skill/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_ActivateAbility::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    if (!AbilityTag.IsValid()) return EStateTreeRunStatus::Failed;

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Owner->FindComponentByClass<UMASkillComponent>();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);
    bool bActivated = SkillComp->TryActivateAbilitiesByTag(TagContainer);
    
    if (!bActivated) return EStateTreeRunStatus::Failed;
    if (!bWaitForAbilityEnd) return EStateTreeRunStatus::Succeeded;

    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMASTTask_ActivateAbility::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    if (!bWaitForAbilityEnd) return EStateTreeRunStatus::Succeeded;

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner) return EStateTreeRunStatus::Failed;

    UMASkillComponent* SkillComp = Owner->FindComponentByClass<UMASkillComponent>();
    if (!SkillComp) return EStateTreeRunStatus::Failed;

    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);
    
    TArray<FGameplayAbilitySpec*> MatchingSpecs;
    SkillComp->GetActivatableGameplayAbilitySpecsByAllMatchingTags(TagContainer, MatchingSpecs);

    for (FGameplayAbilitySpec* Spec : MatchingSpecs)
    {
        if (Spec && Spec->IsActive()) return EStateTreeRunStatus::Running;
    }

    return EStateTreeRunStatus::Succeeded;
}

void FMASTTask_ActivateAbility::ExitState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    if (Transition.CurrentRunStatus == EStateTreeRunStatus::Running)
    {
        AActor* Owner = Cast<AActor>(Context.GetOwner());
        if (Owner)
        {
            if (UMASkillComponent* SkillComp = Owner->FindComponentByClass<UMASkillComponent>())
            {
                FGameplayTagContainer TagContainer;
                TagContainer.AddTag(AbilityTag);
                SkillComp->CancelAbilities(&TagContainer);
            }
        }
    }
}
