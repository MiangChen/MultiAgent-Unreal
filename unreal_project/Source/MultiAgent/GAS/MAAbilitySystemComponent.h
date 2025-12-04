// MAAbilitySystemComponent.h
// GAS 核心组件，挂载到 Agent 上管理所有 Ability

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "MAAbilitySystemComponent.generated.h"

UCLASS()
class MULTIAGENT_API UMAAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UMAAbilitySystemComponent();

    // 初始化默认 Abilities
    void InitializeAbilities(AActor* InOwnerActor);

    // 直接激活 Pickup Ability (推荐使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivatePickup();

    // 直接激活 Drop Ability (推荐使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateDrop();

    // 直接激活 Navigate Ability (推荐使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateNavigate(FVector TargetLocation);

    // 取消导航
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void CancelNavigate();

    // 直接激活 Follow Ability (追踪目标 Character)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateFollow(class AMACharacter* TargetCharacter, float FollowDistance = 200.f);

    // 取消追踪
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void CancelFollow();

    // 直接激活 TakePhoto Ability (Camera 使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateTakePhoto();

    // 通过 Tag 激活 Ability (StateTree 使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateAbilityByTag(FGameplayTag AbilityTag);

    // 通过 Tag 取消 Ability (StateTree 使用)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void CancelAbilityByTag(FGameplayTag AbilityTag);

    // 检查是否有某个 Tag
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool HasGameplayTagFromContainer(FGameplayTag TagToCheck) const;

protected:
    // 默认授予的 Ability 类列表
    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // 保存的 Ability Handle，用于直接激活
    FGameplayAbilitySpecHandle PickupAbilityHandle;
    FGameplayAbilitySpecHandle DropAbilityHandle;
    FGameplayAbilitySpecHandle NavigateAbilityHandle;
    FGameplayAbilitySpecHandle FollowAbilityHandle;
    FGameplayAbilitySpecHandle TakePhotoAbilityHandle;
};
