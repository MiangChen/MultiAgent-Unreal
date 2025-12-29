// SK_TakeOff.h
// 起飞技能 - UAV 从地面起飞到指定高度

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
    void UpdateTakeOff();

private:
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    FVector StartLocation;
    FVector TargetLocation;
    float TakeOffSpeed = 200.f;
    
    FTimerHandle UpdateTimerHandle;
    
    // 结果缓存（用于 EndAbility 生成反馈消息）
    bool bTakeOffSucceeded = false;
    FString TakeOffResultMessage;
};
