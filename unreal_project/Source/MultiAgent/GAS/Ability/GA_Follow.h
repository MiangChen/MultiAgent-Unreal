// GA_Follow.h
// 追踪技能 - 持续跟随目标 Character (使用 Ground Truth 位置)

#pragma once

#include "CoreMinimal.h"
#include "../MAGameplayAbilityBase.h"
#include "GA_Follow.generated.h"

class AMACharacter;

UCLASS()
class MULTIAGENT_API UGA_Follow : public UMAGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UGA_Follow();

    // 设置追踪目标
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetTargetCharacter(AMACharacter* InTargetCharacter);

    // 跟随距离 (与目标保持的距离)
    UPROPERTY(EditDefaultsOnly, Category = "Follow")
    float FollowDistance = 200.f;

    // 更新间隔 (秒)
    UPROPERTY(EditDefaultsOnly, Category = "Follow")
    float UpdateInterval = 0.5f;

    // 重新导航的距离阈值 (目标移动超过此距离才重新导航)
    UPROPERTY(EditDefaultsOnly, Category = "Follow")
    float Repath_Threshold = 100.f;

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
    // 追踪目标
    UPROPERTY()
    TWeakObjectPtr<AMACharacter> TargetCharacter;

    // 上次导航的目标位置
    FVector LastTargetLocation;

    // 定时器句柄
    FTimerHandle UpdateTimerHandle;

    // 定时更新追踪
    void UpdateFollow();

    // 计算跟随位置
    FVector CalculateFollowLocation() const;

    // 缓存
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
};
