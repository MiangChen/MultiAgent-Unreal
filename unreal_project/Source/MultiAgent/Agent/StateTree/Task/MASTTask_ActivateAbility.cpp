// MASTTask_ActivateAbility.cpp

#include "MASTTask_ActivateAbility.h"
#include "../MAStateTreeComponent.h"
#include "MAAbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FMASTTask_ActivateAbility::EnterState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    if (!AbilityTag.IsValid())
    {
        return EStateTreeRunStatus::Failed;
    }

    // 获取 Owner Actor
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner)
    {
        return EStateTreeRunStatus::Failed;
    }

    // 获取 GAS 组件
    UMAAbilitySystemComponent* ASC = Owner->FindComponentByClass<UMAAbilitySystemComponent>();
    if (!ASC)
    {
        return EStateTreeRunStatus::Failed;
    }

    // 激活 Ability
    bool bActivated = ASC->TryActivateAbilityByTag(AbilityTag);
    
    if (!bActivated)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to activate ability: %s"), *AbilityTag.ToString());
        return EStateTreeRunStatus::Failed;
    }

    // 如果不需要等待，直接成功
    if (!bWaitForAbilityEnd)
    {
        return EStateTreeRunStatus::Succeeded;
    }

    // 需要等待 Ability 完成
    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMASTTask_ActivateAbility::Tick(
    FStateTreeExecutionContext& Context, 
    const float DeltaTime) const
{
    if (!bWaitForAbilityEnd)
    {
        return EStateTreeRunStatus::Succeeded;
    }

    // 检查 Ability 是否还在运行
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner)
    {
        return EStateTreeRunStatus::Failed;
    }

    UMAAbilitySystemComponent* ASC = Owner->FindComponentByClass<UMAAbilitySystemComponent>();
    if (!ASC)
    {
        return EStateTreeRunStatus::Failed;
    }

    // 检查是否有匹配的 Ability 正在运行
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);
    
    TArray<FGameplayAbilitySpec*> MatchingSpecs;
    ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(TagContainer, MatchingSpecs);

    for (FGameplayAbilitySpec* Spec : MatchingSpecs)
    {
        if (Spec && Spec->IsActive())
        {
            return EStateTreeRunStatus::Running;
        }
    }

    // Ability 已完成
    return EStateTreeRunStatus::Succeeded;
}

void FMASTTask_ActivateAbility::ExitState(
    FStateTreeExecutionContext& Context, 
    const FStateTreeTransitionResult& Transition) const
{
    // 如果状态被中断，取消 Ability
    if (Transition.CurrentRunStatus == EStateTreeRunStatus::Running)
    {
        AActor* Owner = Cast<AActor>(Context.GetOwner());
        if (Owner)
        {
            if (UMAAbilitySystemComponent* ASC = Owner->FindComponentByClass<UMAAbilitySystemComponent>())
            {
                ASC->CancelAbilityByTag(AbilityTag);
            }
        }
    }
}
