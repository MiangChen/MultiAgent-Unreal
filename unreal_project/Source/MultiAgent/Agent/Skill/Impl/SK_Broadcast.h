// SK_Broadcast.h
// 喊话技能

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "Components/TextRenderComponent.h"
#include "SK_Broadcast.generated.h"

UCLASS()
class MULTIAGENT_API USK_Broadcast : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Broadcast();

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

private:
    FTimerHandle BroadcastTimerHandle;
    float BroadcastDuration = 3.0f;  // 喊话持续时间

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    UPROPERTY()
    UTextRenderComponent* TextBubble;

    void OnBroadcastComplete();
    void ShowBroadcastEffect(const FString& Message);
    void HideBroadcastEffect();
};
