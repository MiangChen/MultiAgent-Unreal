// SK_TakeOff.h
// 起飞技能 - 使用 MANavigationService 统一接口

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_TakeOff.generated.h"

UCLASS()
class MULTIAGENT_API USK_TakeOff : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_TakeOff();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);
    
    void FailAndEnd(const FString& Message);

private:
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    float TargetAltitude = 0.f;
    bool bTakeOffSucceeded = false;
    FString TakeOffResultMessage;
};
