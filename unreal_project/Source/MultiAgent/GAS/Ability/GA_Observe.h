// GA_Observe.h
// 观察技能 - Sensor 持续观察目标

#pragma once

#include "CoreMinimal.h"
#include "../MAGameplayAbilityBase.h"
#include "GA_Observe.generated.h"

UCLASS()
class MULTIAGENT_API UGA_Observe : public UMAGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UGA_Observe();

    // 设置观察目标
    UFUNCTION(BlueprintCallable, Category = "Observe")
    void SetObserveTarget(AActor* InTarget);

    // 观察范围
    UPROPERTY(EditDefaultsOnly, Category = "Observe")
    float ObserveRange = 1500.f;

    // 更新间隔
    UPROPERTY(EditDefaultsOnly, Category = "Observe")
    float UpdateInterval = 0.3f;

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
    // 观察目标
    UPROPERTY()
    TWeakObjectPtr<AActor> ObserveTarget;

    // 定时器句柄
    FTimerHandle ObserveTimerHandle;
    
    // 缓存
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    // 更新观察状态
    void UpdateObservation();
    
    // 目标丢失时的处理
    void OnTargetLost();
    
    // 清理
    void CleanupObserve();
};
