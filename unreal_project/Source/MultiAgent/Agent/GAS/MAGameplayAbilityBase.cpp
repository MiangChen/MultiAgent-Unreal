// MAGameplayAbilityBase.cpp

#include "MAGameplayAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "../Character/MACharacter.h"

UMAGameplayAbilityBase::UMAGameplayAbilityBase()
{
    // 默认设置：实例化策略
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

AMACharacter* UMAGameplayAbilityBase::GetOwningCharacter() const
{
    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (ActorInfo && ActorInfo->AvatarActor.IsValid())
    {
        return Cast<AMACharacter>(ActorInfo->AvatarActor.Get());
    }
    return nullptr;
}

void UMAGameplayAbilityBase::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UMAGameplayAbilityBase::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UMAGameplayAbilityBase::BroadcastGameplayEvent(FGameplayTag EventTag)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        FGameplayEventData EventData;
        EventData.Instigator = GetOwningCharacter();
        ASC->HandleGameplayEvent(EventTag, &EventData);
    }
}
