// SK_Navigate.h
// 导航技能 - 委托到 NavigationService 实现

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Navigate.generated.h"

class UMANavigationService;

UCLASS()
class MULTIAGENT_API USK_Navigate : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Navigate();

    UFUNCTION(BlueprintCallable, Category = "Navigate")
    void SetTargetLocation(FVector InTargetLocation);

    UPROPERTY(EditDefaultsOnly, Category = "Navigate")
    float AcceptanceRadius = 200.f;

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

private:
    FVector TargetLocation;

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    // 导航结果缓存（用于 EndAbility 生成反馈消息）
    bool bNavigationSucceeded = false;
    FString NavigationResultMessage;
    
    // NavigationService 引用
    UPROPERTY()
    UMANavigationService* NavigationService = nullptr;
    
    // 导航完成回调
    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);
};
