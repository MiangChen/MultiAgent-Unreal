// MASTTask_ReturnHome.h
// StateTree Task: 返航到出生点并降落

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MASTTask_ReturnHome.generated.h"

USTRUCT()
struct FMASTTask_ReturnHomeInstanceData
{
    GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "MA ReturnHome"))
struct MULTIAGENT_API FMASTTask_ReturnHome : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

protected:
    virtual const UStruct* GetInstanceDataType() const override { return FMASTTask_ReturnHomeInstanceData::StaticStruct(); }
    
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
