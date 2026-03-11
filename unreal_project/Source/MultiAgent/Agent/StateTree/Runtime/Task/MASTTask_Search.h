// MASTTask_Search.h
// StateTree Task: 搜索任务

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MASTTask_Search.generated.h"

USTRUCT()
struct FMASTTask_SearchInstanceData
{
    GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "MA Search"))
struct MULTIAGENT_API FMASTTask_Search : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

protected:
    virtual const UStruct* GetInstanceDataType() const override { return FMASTTask_SearchInstanceData::StaticStruct(); }
    
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
