// MASTCondition_LowEnergy.h
// StateTree Condition: 检查电量是否低于阈值

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "MASTCondition_LowEnergy.generated.h"

// 空的 Instance Data (StateTree 要求必须有)
USTRUCT()
struct FMASTCondition_LowEnergyInstanceData
{
    GENERATED_BODY()
};

/**
 * 检查 Quadruped 电量是否低于阈值
 */
USTRUCT(meta = (DisplayName = "MA Low Energy"))
struct MULTIAGENT_API FMASTCondition_LowEnergy : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()

    // 电量阈值 (百分比)
    UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0", ClampMax = "100"))
    float EnergyThreshold = 20.f;

protected:
    virtual const UStruct* GetInstanceDataType() const override 
    { 
        return FMASTCondition_LowEnergyInstanceData::StaticStruct(); 
    }
    
    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
