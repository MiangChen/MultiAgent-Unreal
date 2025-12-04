// MASTTask_Patrol.h
// StateTree Task: 巡逻 - 循环移动到路径点

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MASTTask_Patrol.generated.h"

class AMAPatrolPath;

// Task 实例数据 (必须在 Task 之前定义)
USTRUCT()
struct FMASTTask_PatrolInstanceData
{
    GENERATED_BODY()

    // 路径点数组
    TArray<FVector> Waypoints;
    
    // 当前路径点索引
    int32 CurrentWaypointIndex = 0;
    
    // 是否正在移动
    bool bIsMoving = false;
    
    // 是否正在等待
    bool bIsWaiting = false;
    
    // 等待计时器
    float WaitTimer = 0.f;
};

/**
 * StateTree Task: 巡逻
 * 循环移动到 PatrolPath 的各个路径点
 * 通过 Actor Tag 在运行时查找 PatrolPath，避免直接引用关卡 Actor
 */
USTRUCT(meta = (DisplayName = "MA Patrol"))
struct MULTIAGENT_API FMASTTask_Patrol : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

    // 巡逻路径的 Actor Tag (在场景中给 PatrolPath Actor 添加此 Tag)
    UPROPERTY(EditAnywhere, Category = "Patrol")
    FName PatrolPathTag = "PatrolPath";

    // 到达路径点后的等待时间
    UPROPERTY(EditAnywhere, Category = "Patrol")
    float WaitTimeAtWaypoint = 0.5f;

    // 到达判定距离
    UPROPERTY(EditAnywhere, Category = "Patrol")
    float AcceptanceRadius = 100.f;

protected:
    virtual const UStruct* GetInstanceDataType() const override { return FMASTTask_PatrolInstanceData::StaticStruct(); }
    
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
