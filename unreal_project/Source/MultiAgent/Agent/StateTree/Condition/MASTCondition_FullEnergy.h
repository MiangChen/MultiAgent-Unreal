// MASTCondition_FullEnergy.h
// StateTree Condition: 检查电量是否已满

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "MASTCondition_FullEnergy.generated.h"

// 空的 Instance Data (StateTree 要求必须有)
USTRUCT()
struct FMASTCondition_FullEnergyInstanceData
{
    GENERATED_BODY()
};

/**
 * 检查 RobotDog 电量是否已满
 */
USTRUCT(meta = (DisplayName = "MA Full Energy"))
struct MULTIAGENT_API FMASTCondition_FullEnergy : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()

    // 满电阈值 (百分比)
    UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0", ClampMax = "100"))
    float FullThreshold = 100.f;

protected:
    virtual const UStruct* GetInstanceDataType() const override 
    { 
        return FMASTCondition_FullEnergyInstanceData::StaticStruct(); 
    }
    
    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
