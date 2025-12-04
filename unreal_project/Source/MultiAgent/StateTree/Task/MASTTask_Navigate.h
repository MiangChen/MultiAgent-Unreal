// MASTTask_Navigate.h
// StateTree Task: 导航到目标位置

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MASTTask_Navigate.generated.h"

// Task 实例数据 (必须在 Task 之前定义)
USTRUCT()
struct FMASTTask_NavigateInstanceData
{
    GENERATED_BODY()

    // 是否已开始导航
    bool bNavigationStarted = false;
    
    // 导航是否完成
    bool bNavigationCompleted = false;
    
    // 导航是否成功
    bool bNavigationSucceeded = false;
};

/**
 * StateTree Task: 导航到指定位置
 * 使用 GA_Navigate 执行移动，等待移动完成后返回成功
 */
USTRUCT(meta = (DisplayName = "MA Navigate To Location"))
struct MULTIAGENT_API FMASTTask_Navigate : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

    // 目标位置 (可绑定到 StateTree 参数)
    UPROPERTY(EditAnywhere, Category = "Navigate")
    FVector TargetLocation = FVector::ZeroVector;

    // 到达判定距离
    UPROPERTY(EditAnywhere, Category = "Navigate")
    float AcceptanceRadius = 100.f;

protected:
    virtual const UStruct* GetInstanceDataType() const override { return FMASTTask_NavigateInstanceData::StaticStruct(); }
    
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
