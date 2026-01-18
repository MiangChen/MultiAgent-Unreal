// SK_TakePhoto.h
// 拍照技能

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_TakePhoto.generated.h"

UCLASS()
class MULTIAGENT_API USK_TakePhoto : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_TakePhoto();

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
    FTimerHandle PhotoTimerHandle;
    float PhotoDuration = 2.0f;  // 拍照持续时间

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    void OnPhotoComplete();
    void ShowPhotoEffect();
};
