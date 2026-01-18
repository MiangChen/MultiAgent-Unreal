// SK_Guide.h
// 引导技能

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "Containers/Ticker.h"
#include "SK_Guide.generated.h"

UCLASS()
class MULTIAGENT_API USK_Guide : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Guide();

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
    TWeakObjectPtr<AActor> GuideTargetActor;
    FVector GuideDestination;
    float AcceptanceRadius = 100.f;
    float FollowDistance = 200.f;  // 被引导对象跟随距离
    float StartTime = 0.f;

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    bool TickGuide(float DeltaTime);
    void UpdateTargetPosition(float DeltaTime);
    void OnGuideComplete();
};
