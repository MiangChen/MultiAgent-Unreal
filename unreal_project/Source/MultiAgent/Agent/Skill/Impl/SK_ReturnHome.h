// SK_ReturnHome.h
// 返航技能 - UAV/FixedWingUAV 返回起始位置并降落

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_ReturnHome.generated.h"

UCLASS()
class MULTIAGENT_API USK_ReturnHome : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_ReturnHome();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    void UpdateReturnHome();
    void StartLanding();
    void UpdateLanding();

private:
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    FVector HomeLocation;
    float LandHeight = 0.f;
    float FlySpeed = 500.f;
    float LandSpeed = 150.f;
    
    bool bIsLanding = false;
    FTimerHandle UpdateTimerHandle;
    
    // 结果缓存（用于 EndAbility 生成反馈消息）
    bool bReturnHomeSucceeded = false;
    FString ReturnHomeResultMessage;
};
