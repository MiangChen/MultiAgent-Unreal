// GA_Navigate.h
// 导航技能 - 异步移动到目标位置

#pragma once

#include "CoreMinimal.h"
#include "../MAGameplayAbilityBase.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "GA_Navigate.generated.h"

UCLASS()
class MULTIAGENT_API UGA_Navigate : public UMAGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UGA_Navigate();

    // 设置导航目标位置 (激活前调用)
    UFUNCTION(BlueprintCallable, Category = "Navigate")
    void SetTargetLocation(FVector InTargetLocation);

    // 到达判定距离
    UPROPERTY(EditDefaultsOnly, Category = "Navigate")
    float AcceptanceRadius = 100.f;

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
    // 目标位置
    FVector TargetLocation;

    // 导航完成回调 (UE 5.5 新签名)
    void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

    // 开始导航
    void StartNavigation();

    // 清理导航状态
    void CleanupNavigation();

    // 缓存的 Handle 用于 EndAbility
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
};
