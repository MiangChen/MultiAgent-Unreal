// MASTTask_Place.h
// StateTree Task: Place 任务 - 搬运物体到目标

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MASTTask_Place.generated.h"

USTRUCT()
struct FMASTTask_PlaceInstanceData
{
    GENERATED_BODY()

    bool bSkillActivated = false;
};

USTRUCT(meta = (DisplayName = "MA Place"))
struct MULTIAGENT_API FMASTTask_Place : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

protected:
    virtual const UStruct* GetInstanceDataType() const override { return FMASTTask_PlaceInstanceData::StaticStruct(); }
    
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
