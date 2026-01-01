// MASTTask_TakeOff.h
// StateTree Task: 起飞

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MASTTask_TakeOff.generated.h"

USTRUCT()
struct FMASTTask_TakeOffInstanceData
{
    GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "MA TakeOff"))
struct MULTIAGENT_API FMASTTask_TakeOff : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

protected:
    virtual const UStruct* GetInstanceDataType() const override { return FMASTTask_TakeOffInstanceData::StaticStruct(); }
    
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
