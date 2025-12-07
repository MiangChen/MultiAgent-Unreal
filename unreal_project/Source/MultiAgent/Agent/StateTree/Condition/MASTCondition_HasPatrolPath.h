// MASTCondition_HasPatrolPath.h
// StateTree Condition: 检查是否有可用的 PatrolPath

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "MASTCondition_HasPatrolPath.generated.h"

class AMAPatrolPath;

// 空的 Instance Data (StateTree 要求必须有)
USTRUCT()
struct FMASTCondition_HasPatrolPathInstanceData
{
    GENERATED_BODY()
};

/**
 * 检查场景中是否有可用的 PatrolPath
 */
USTRUCT(meta = (DisplayName = "MA Has Patrol Path"))
struct MULTIAGENT_API FMASTCondition_HasPatrolPath : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()

    // PatrolPath 的 Actor Tag (用于运行时查找)
    UPROPERTY(EditAnywhere, Category = "Condition")
    FName PatrolPathTag = "PatrolPath";

protected:
    virtual const UStruct* GetInstanceDataType() const override 
    { 
        return FMASTCondition_HasPatrolPathInstanceData::StaticStruct(); 
    }
    
    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
