// MASTTask_Navigate.h
// StateTree Task: 导航到目标位置

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MASTTask_Navigate.generated.h"

USTRUCT()
struct FMASTTask_NavigateInstanceData
{
    GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "MA Navigate"))
struct MULTIAGENT_API FMASTTask_Navigate : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

protected:
    virtual const UStruct* GetInstanceDataType() const override { return FMASTTask_NavigateInstanceData::StaticStruct(); }
    
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
