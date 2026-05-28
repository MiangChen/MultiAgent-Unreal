// MASTTask_Land.h
// StateTree Task: 降落

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MASTTask_Land.generated.h"

USTRUCT()
struct FMASTTask_LandInstanceData
{
    GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "MA Land"))
struct MULTIAGENT_API FMASTTask_Land : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

protected:
    virtual const UStruct* GetInstanceDataType() const override { return FMASTTask_LandInstanceData::StaticStruct(); }
    
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
