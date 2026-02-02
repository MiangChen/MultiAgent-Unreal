// SK_Follow.h
// 跟随技能 - 支持跟随任意场景对象
// 使用 NavigationService 的跟随模式进行平滑跟随

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Follow.generated.h"

class UMANavigationService;

UCLASS()
class MULTIAGENT_API USK_Follow : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Follow();
    
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetTargetActor(AActor* InTargetActor);
    
    void SetFollowDistance(float InDistance) { FollowDistance = InDistance; }

    /** 跟随距离 (从 ConfigManager 加载，可覆盖) */
    UPROPERTY(EditDefaultsOnly, Category = "Follow")
    float FollowDistance = 300.f;
    
    /** 状态检查间隔 (秒) */
    UPROPERTY(EditDefaultsOnly, Category = "Follow")
    float UpdateInterval = 0.5f;

    /** 持续跟踪时间阈值 (秒) */
    UPROPERTY(EditDefaultsOnly, Category = "Follow")
    float ContinuousFollowTimeThreshold = 30.f;

    /** 跟随位置容差 (从 ConfigManager 加载，可覆盖) */
    UPROPERTY(EditDefaultsOnly, Category = "Follow")
    float FollowPositionTolerance = 200.f;

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor;
    
    FTimerHandle UpdateTimerHandle;
    
    void UpdateFollow();

    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    UPROPERTY()
    UMANavigationService* NavigationService = nullptr;

    float ContinuousFollowTime = 0.f;
    
    bool bFollowSucceeded = false;
    FString FollowResultMessage;
};
