// SK_HandleHazard.h
// 处理危险技能

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "Containers/Ticker.h"
#include "SK_HandleHazard.generated.h"

UCLASS()
class MULTIAGENT_API USK_HandleHazard : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_HandleHazard();

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
    FTSTicker::FDelegateHandle TickDelegateHandle;
    float HandleDuration = 5.0f;  // 处理持续时间
    float HandleProgress = 0.f;   // 处理进度 (0.0 - 1.0)
    float StartTime = 0.f;        // 开始时间

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    bool TickHandle(float DeltaTime);
    void ShowHandleEffect();
    void UpdateProgressDisplay();
    void OnHandleComplete();
};
