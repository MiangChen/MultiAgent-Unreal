// GA_Charge.h
// 充电技能 - 在充电站渐进式恢复电量

#pragma once

#include "CoreMinimal.h"
#include "../MAGameplayAbilityBase.h"
#include "GA_Charge.generated.h"

class AMAChargingStation;

UCLASS()
class MULTIAGENT_API UGA_Charge : public UMAGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UGA_Charge();

    // 充电速率（每秒恢复的电量百分比，基于 MaxEnergy）
    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    float ChargeRatePerSecond = 20.f;

    // 充电更新间隔（秒）
    UPROPERTY(EditDefaultsOnly, Category = "Charge")
    float ChargeUpdateInterval = 0.5f;

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

    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

private:
    // 缓存
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    // 当前充电站
    UPROPERTY()
    TWeakObjectPtr<AMAChargingStation> CurrentChargingStation;

    // 定时器句柄
    FTimerHandle ChargeTimerHandle;

    // 查找附近的充电站
    AMAChargingStation* FindNearbyChargingStation() const;
    
    // 定时充电更新
    void UpdateCharge();
};
