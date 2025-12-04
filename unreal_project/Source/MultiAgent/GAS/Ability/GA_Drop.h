// GA_Drop.h
// 放下物品技能 - UE 5.5 风格

#pragma once

#include "CoreMinimal.h"
#include "../MAGameplayAbilityBase.h"
#include "GA_Drop.generated.h"

class AMAPickupItem;

UCLASS()
class MULTIAGENT_API UGA_Drop : public UMAGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UGA_Drop();

    // 放下位置偏移 (相对于角色前方)
    UPROPERTY(EditDefaultsOnly, Category = "Drop")
    float DropForwardOffset = 100.f;

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

private:
    // 查找当前持有的物品
    AMAPickupItem* FindHeldItem() const;

    // 执行放下
    void PerformDrop(AMAPickupItem* Item);
};
