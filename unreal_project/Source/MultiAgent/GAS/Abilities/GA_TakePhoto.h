// GA_TakePhoto.h
// 拍照技能 - 瞬时截取当前帧

#pragma once

#include "CoreMinimal.h"
#include "../MAGameplayAbilityBase.h"
#include "GA_TakePhoto.generated.h"

class AMACameraAgent;

UCLASS()
class MULTIAGENT_API UGA_TakePhoto : public UMAGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UGA_TakePhoto();

    // 保存路径（空则自动生成）
    UPROPERTY(EditDefaultsOnly, Category = "TakePhoto")
    FString SavePath;

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
    // 获取 Camera Agent
    AMACameraAgent* GetCameraAgent() const;
};
