// SK_Place.h
// Place 技能 - 搬运物体到目标位置

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Place.generated.h"

class AMAPickupItem;

UENUM()
enum class EPlacePhase : uint8
{
    MoveToObject,
    PickUp,
    MoveToTarget,
    PutDown,
    Complete
};

UCLASS()
class MULTIAGENT_API USK_Place : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Place();

    UPROPERTY(EditDefaultsOnly, Category = "Place")
    float InteractionRadius = 150.f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Place")
    float BendDuration = 0.8f;

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    void UpdatePhase();
    void PerformPickup();
    void PerformPutDown();
    
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    EPlacePhase CurrentPhase = EPlacePhase::MoveToObject;
    FTimerHandle PhaseTimerHandle;
    
    UPROPERTY()
    TWeakObjectPtr<AActor> TargetObject;
    
    UPROPERTY()
    TWeakObjectPtr<AActor> HeldObject;
    
    FVector TargetLocation;
    FVector DropLocation;
    
    // 结果缓存（用于 EndAbility 生成反馈消息）
    bool bPlaceSucceeded = false;
    FString PlaceResultMessage;
};
