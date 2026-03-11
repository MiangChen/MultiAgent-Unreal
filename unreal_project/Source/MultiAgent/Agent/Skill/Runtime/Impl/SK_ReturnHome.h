// SK_ReturnHome.h
// 返航技能 - 使用 MANavigationService 统一接口

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_ReturnHome.generated.h"

UCLASS()
class MULTIAGENT_API USK_ReturnHome : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_ReturnHome();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);
    
    void FailAndEnd(const FString& Message);

private:
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    FVector HomeLocation;
    float LandAltitude = 0.f;
    bool bReturnHomeSucceeded = false;
    FString ReturnHomeResultMessage;
};
