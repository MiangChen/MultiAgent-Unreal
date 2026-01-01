// SK_Follow.h
// 跟随技能

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Follow.generated.h"

class AMACharacter;

UCLASS()
class MULTIAGENT_API USK_Follow : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Follow();

    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetTargetCharacter(AMACharacter* InTargetCharacter);
    
    void SetFollowDistance(float InDistance) { FollowDistance = InDistance; }

    UPROPERTY(EditDefaultsOnly, Category = "Follow")
    float FollowDistance = 200.f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Follow")
    float UpdateInterval = 0.5f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Follow")
    float RepathThreshold = 100.f;

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    UPROPERTY()
    TWeakObjectPtr<AMACharacter> TargetCharacter;
    
    FVector LastTargetLocation;
    FTimerHandle UpdateTimerHandle;
    
    void UpdateFollow();
    FVector CalculateFollowLocation() const;

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    // 结果缓存（用于 EndAbility 生成反馈消息）
    bool bFollowSucceeded = false;
    FString FollowResultMessage;
};
