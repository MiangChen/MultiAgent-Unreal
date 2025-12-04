// GA_Avoid.h
// 避障技能 - 检测并绕开障碍物

#pragma once

#include "CoreMinimal.h"
#include "../MAGameplayAbilityBase.h"
#include "GA_Avoid.generated.h"

UCLASS()
class MULTIAGENT_API UGA_Avoid : public UMAGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UGA_Avoid();

    // 检测半径
    UPROPERTY(EditDefaultsOnly, Category = "Avoid")
    float DetectionRadius = 200.f;

    // 避障力度
    UPROPERTY(EditDefaultsOnly, Category = "Avoid")
    float AvoidanceStrength = 1.f;

    // 检测间隔
    UPROPERTY(EditDefaultsOnly, Category = "Avoid")
    float CheckInterval = 0.1f;

    // 设置目标位置 (避障时保持向目标前进)
    UFUNCTION(BlueprintCallable, Category = "Avoid")
    void SetDestination(FVector InDestination);

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
    // 原始目标位置
    FVector OriginalDestination;
    
    // 定时器句柄
    FTimerHandle AvoidTimerHandle;
    
    // 缓存
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    // 检查障碍物
    void CheckObstacles();
    
    // 计算避障向量
    FVector CalculateAvoidanceVector(const TArray<AActor*>& Obstacles);
    
    // 应用避障
    void ApplyAvoidance(FVector AvoidanceDir);
    
    // 清理
    void CleanupAvoid();
};
