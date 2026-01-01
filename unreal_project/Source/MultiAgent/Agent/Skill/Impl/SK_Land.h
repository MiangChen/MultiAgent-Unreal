// SK_Land.h
// 降落技能 - UAV 从当前高度降落到地面

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Land.generated.h"

UCLASS()
class MULTIAGENT_API USK_Land : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Land();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    void UpdateLand();

private:
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    FVector StartLocation;
    FVector TargetLocation;
    float LandSpeed = 150.f;
    
    FTimerHandle UpdateTimerHandle;
    
    // 结果缓存（用于 EndAbility 生成反馈消息）
    bool bLandSucceeded = false;
    FString LandResultMessage;
};
