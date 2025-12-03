// MAGameplayAbilityBase.h
// 技能基类，所有 GA_XXX 继承此类

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MAGameplayAbilityBase.generated.h"

class AMAAgent;

UCLASS()
class MULTIAGENT_API UMAGameplayAbilityBase : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UMAGameplayAbilityBase();

    // 获取拥有此 Ability 的 Agent
    UFUNCTION(BlueprintCallable, Category = "Ability")
    AMAAgent* GetOwningAgent() const;

protected:
    // 技能激活时
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    // 技能结束时
    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

    // 发送 Gameplay Event (用于通知 State Tree)
    // 使用不同的函数名避免隐藏父类虚函数
    void BroadcastGameplayEvent(FGameplayTag EventTag);
};
