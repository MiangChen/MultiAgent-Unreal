// MASkillBase.h
// 技能基类，所有技能继承此类

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MASkillBase.generated.h"

class AMACharacter;

UCLASS()
class MULTIAGENT_API UMASkillBase : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UMASkillBase();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    AMACharacter* GetOwningCharacter() const;

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

    void BroadcastGameplayEvent(FGameplayTag EventTag);
};
