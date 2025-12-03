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

    // 通过 Tag 激活 Ability
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool TryActivateAbilityByTag(FGameplayTag AbilityTag);

    // 通过 Tag 取消 Ability
    UFUNCTION(BlueprintCallable, Category = "GAS")
    void CancelAbilityByTag(FGameplayTag AbilityTag);

    // 检查是否有某个 Tag (使用不同的函数名避免隐藏父类方法)
    UFUNCTION(BlueprintCallable, Category = "GAS")
    bool HasGameplayTagFromContainer(FGameplayTag TagToCheck) const;

protected:
    // 默认授予的 Ability 类列表
    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;
};
