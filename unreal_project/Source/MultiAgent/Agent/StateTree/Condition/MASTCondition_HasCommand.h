// MASTCondition_HasCommand.h
// StateTree Condition: 检查是否收到指定命令

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "GameplayTagContainer.h"
#include "MASTCondition_HasCommand.generated.h"

// 空的 Instance Data (StateTree 要求必须有)
USTRUCT()
struct FMASTCondition_HasCommandInstanceData
{
    GENERATED_BODY()
};

/**
 * 检查 Character 是否收到指定的命令 Tag
 * 用于外部控制 StateTree 状态切换
 */
USTRUCT(meta = (DisplayName = "MA Has Command"))
struct MULTIAGENT_API FMASTCondition_HasCommand : public FStateTreeConditionCommonBase
{
    GENERATED_BODY()

    // 要检查的命令 Tag (如 Command.Patrol, Command.Charge)
    UPROPERTY(EditAnywhere, Category = "Condition")
    FGameplayTag CommandTag;

protected:
    virtual const UStruct* GetInstanceDataType() const override 
    { 
        return FMASTCondition_HasCommandInstanceData::StaticStruct(); 
    }
    
    virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
