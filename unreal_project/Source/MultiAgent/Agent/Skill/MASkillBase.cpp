// MASkillBase.cpp

#include "MASkillBase.h"
#include "AbilitySystemComponent.h"
#include "../Character/MACharacter.h"

UMASkillBase::UMASkillBase()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

AMACharacter* UMASkillBase::GetOwningCharacter() const
{
    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (ActorInfo && ActorInfo->AvatarActor.IsValid())
    {
        return Cast<AMACharacter>(ActorInfo->AvatarActor.Get());
    }
    return nullptr;
}

void UMASkillBase::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UMASkillBase::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UMASkillBase::BroadcastGameplayEvent(FGameplayTag EventTag)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        FGameplayEventData EventData;
        EventData.Instigator = GetOwningCharacter();
        ASC->HandleGameplayEvent(EventTag, &EventData);
    }
}
