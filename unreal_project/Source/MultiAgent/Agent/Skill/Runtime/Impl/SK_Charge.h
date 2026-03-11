// SK_Charge.h
// 充电技能

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Charge.generated.h"

class AMAChargingStation;

UCLASS()
class MULTIAGENT_API USK_Charge : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Charge();

    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    float ChargeRatePerSecond = 20.f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    float ChargeUpdateInterval = 0.5f;

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

private:
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    UPROPERTY()
    TWeakObjectPtr<AMAChargingStation> CurrentChargingStation;
    
    FTimerHandle ChargeTimerHandle;
    
    AMAChargingStation* FindNearbyChargingStation() const;
    void UpdateCharge();
    
    // 结果缓存（用于 EndAbility 生成反馈消息）
    bool bChargeSucceeded = false;
    FString ChargeResultMessage;
};
