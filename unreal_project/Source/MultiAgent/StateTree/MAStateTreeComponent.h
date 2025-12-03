// MAStateTreeComponent.h
// State Tree 组件，挂载到 Agent 上管理状态机

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeComponent.h"
#include "MAStateTreeComponent.generated.h"

class UMAAbilitySystemComponent;

/**
 * Agent 的 State Tree 组件
 * 
 * 职责：
 * - 管理 Agent 的高层状态 (Exploration, PhotoMode, Interaction)
 * - 监听 GAS 事件，触发状态转换
 * - 向 GAS 发送激活 Ability 的请求
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MULTIAGENT_API UMAStateTreeComponent : public UStateTreeComponent
{
    GENERATED_BODY()

public:
    UMAStateTreeComponent();

    // 获取关联的 GAS 组件
    UFUNCTION(BlueprintCallable, Category = "StateTree")
    UMAAbilitySystemComponent* GetAbilitySystemComponent() const;

    // 通过 Tag 请求激活 Ability
    UFUNCTION(BlueprintCallable, Category = "StateTree")
    bool RequestActivateAbility(FGameplayTag AbilityTag);

    // 通过 Tag 请求取消 Ability
    UFUNCTION(BlueprintCallable, Category = "StateTree")
    void RequestCancelAbility(FGameplayTag AbilityTag);

protected:
    virtual void BeginPlay() override;

private:
    // 缓存的 GAS 组件引用
    UPROPERTY()
    TWeakObjectPtr<UMAAbilitySystemComponent> CachedASC;
};
