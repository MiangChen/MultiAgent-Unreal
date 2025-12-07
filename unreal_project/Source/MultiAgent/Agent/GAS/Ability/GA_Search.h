// GA_Search.h
// 搜索技能 - 使用摄像头检测 Human

#pragma once

#include "CoreMinimal.h"
#include "../MAGameplayAbilityBase.h"
#include "GA_Search.generated.h"

class AMACameraSensor;
class AMAHumanCharacter;

UCLASS()
class MULTIAGENT_API UGA_Search : public UMAGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UGA_Search();

    // 检测间隔 (秒)
    UPROPERTY(EditDefaultsOnly, Category = "Search")
    float DetectionInterval = 0.5f;

    // 检测范围
    UPROPERTY(EditDefaultsOnly, Category = "Search")
    float DetectionRange = 1000.f;

    // 检测视野角度 (度)
    UPROPERTY(EditDefaultsOnly, Category = "Search")
    float DetectionFOV = 90.f;

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
    // 定时器句柄
    FTimerHandle SearchTimerHandle;
    
    // 缓存
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    // 执行检测
    void PerformDetection();
    
    // 检测视野内的 Human
    bool DetectHumanInView(AMAHumanCharacter*& OutHuman);
    
    // 发现目标时的处理
    void OnTargetFound(AMAHumanCharacter* Human);
    
    // 获取附着的摄像头
    AMACameraSensor* GetAttachedCamera() const;
    
    // 清理
    void CleanupSearch();
};
