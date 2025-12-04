// MASTTask_ActivateAbility.h
// State Tree Task: 激活 GAS Ability

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayTagContainer.h"
#include "MASTTask_ActivateAbility.generated.h"

/**
 * State Tree Task: 激活指定的 Gameplay Ability
 * 
 * 用法：在 State Tree 的某个状态进入时，激活对应的 GAS Ability
 * 例如：进入 Pickup 状态时，激活 GA_Pickup
 */
USTRUCT(meta = (DisplayName = "MA Activate Ability"))
struct MULTIAGENT_API FMASTTask_ActivateAbility : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

    // 要激活的 Ability Tag
    UPROPERTY(EditAnywhere, Category = "Ability")
    FGameplayTag AbilityTag;

    // 是否等待 Ability 完成
    UPROPERTY(EditAnywhere, Category = "Ability")
    bool bWaitForAbilityEnd = false;

protected:
    virtual const UStruct* GetInstanceDataType() const override { return nullptr; }
    
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
