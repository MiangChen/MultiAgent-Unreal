// GA_Pickup.h
// 拾取技能 - UE 5.5 风格

#pragma once

#include "CoreMinimal.h"
#include "../MAGameplayAbilityBase.h"
#include "GA_Pickup.generated.h"

class AMAPickupItem;

UCLASS()
class MULTIAGENT_API UGA_Pickup : public UMAGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UGA_Pickup();

    // 检测范围 (应大于 MAPickupItem 的 CollisionComponent 半径 100)
    UPROPERTY(EditDefaultsOnly, Category = "Pickup")
    float PickupRadius = 200.f;

    // 附着的骨骼 Socket 名称 (手部)
    UPROPERTY(EditDefaultsOnly, Category = "Pickup")
    FName AttachSocketName = FName("hand_r");

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
    // 查找最近的可拾取物品
    AMAPickupItem* FindNearestPickupItem() const;

    // 执行拾取
    void PerformPickup(AMAPickupItem* Item);
};
