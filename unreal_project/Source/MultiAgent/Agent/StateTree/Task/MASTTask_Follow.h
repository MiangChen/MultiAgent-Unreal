// MASTTask_Follow.h
// StateTree Task: 跟随目标

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MASTTask_Follow.generated.h"

class AMACharacter;

USTRUCT()
struct FMASTTask_FollowInstanceData
{
    GENERATED_BODY()

    // 跟随目标
    UPROPERTY()
    TWeakObjectPtr<AMACharacter> TargetCharacter;

    // 是否已激活 Follow ability
    bool bAbilityActivated = false;
};

/**
 * StateTree Task: 跟随目标
 * 机器人持续跟随指定目标
 */
USTRUCT(meta = (DisplayName = "MA Follow"))
struct MULTIAGENT_API FMASTTask_Follow : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

    // 注意: 跟随目标从 Robot 的 FollowTarget 获取
    // 注意: 跟随距离从 Robot 的 ScanRadius 获取

protected:
    virtual const UStruct* GetInstanceDataType() const override { return FMASTTask_FollowInstanceData::StaticStruct(); }

    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
