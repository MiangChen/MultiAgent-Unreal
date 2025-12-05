// MASTTask_Coverage.h
// StateTree Task: 区域覆盖扫描（割草机模式）

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MASTTask_Coverage.generated.h"

class AMACoverageArea;

USTRUCT()
struct FMASTTask_CoverageInstanceData
{
    GENERATED_BODY()

    // 当前路径点索引
    int32 CurrentPathIndex = 0;

    // 总路径点数
    int32 TotalPathPoints = 0;

    // 是否正在移动
    bool bIsMoving = false;

    // 是否已完成
    bool bIsCompleted = false;

    // 缓存的覆盖路径
    TArray<FVector> CoveragePath;

    // 引用的覆盖区域（用于可视化）
    UPROPERTY()
    TWeakObjectPtr<AMACoverageArea> CoverageAreaRef;
};

/**
 * StateTree Task: 区域覆盖扫描
 * 机器人按"割草机"模式覆盖指定区域
 * 
 * 注意: CoverageArea 从 Robot->GetCoverageArea() 获取
 * 注意: ScanRadius 从 Robot->ScanRadius 获取
 */
USTRUCT(meta = (DisplayName = "MA Coverage"))
struct MULTIAGENT_API FMASTTask_Coverage : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

protected:
    virtual const UStruct* GetInstanceDataType() const override { return FMASTTask_CoverageInstanceData::StaticStruct(); }

    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
    // 移动到下一个路径点
    bool MoveToNextPoint(FStateTreeExecutionContext& Context, FMASTTask_CoverageInstanceData& Data) const;
};
