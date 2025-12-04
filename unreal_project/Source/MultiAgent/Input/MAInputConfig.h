// MAInputConfig.h
// 输入配置数据资产 - 定义 InputAction 与 GameplayTag 的映射

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MAInputConfig.generated.h"

class UInputAction;

/**
 * 输入动作与 Tag 的映射
 */
USTRUCT(BlueprintType)
struct FMAInputAction
{
    GENERATED_BODY()

    // 输入动作
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TObjectPtr<UInputAction> InputAction = nullptr;

    // 对应的 Gameplay Tag
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTag InputTag;
};

/**
 * 输入配置数据资产
 * 在编辑器中创建并配置输入映射
 */
UCLASS(BlueprintType, Const)
class MULTIAGENT_API UMAInputConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    // 根据 Tag 查找 InputAction
    UFUNCTION(BlueprintCallable, Category = "Input")
    const UInputAction* FindInputActionByTag(const FGameplayTag& InputTag) const;

    // 原生输入动作列表（点击、移动等）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TArray<FMAInputAction> NativeInputActions;

    // 技能输入动作列表（拾取、放下等）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TArray<FMAInputAction> AbilityInputActions;
};
