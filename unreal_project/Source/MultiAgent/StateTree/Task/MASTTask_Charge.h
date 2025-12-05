// MASTTask_Charge.h
// StateTree Task: 自动导航到充电站并充电

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "MASTTask_Charge.generated.h"

class AMAChargingStation;

USTRUCT()
struct FMASTTask_ChargeInstanceData
{
    GENERATED_BODY()

    // 是否已找到充电站
    bool bFoundStation = false;
    
    // 是否正在移动
    bool bIsMoving = false;
    
    // 是否正在充电
    bool bIsCharging = false;
    
    // 充电站位置
    FVector StationLocation = FVector::ZeroVector;

    // 充电站引用
    UPROPERTY()
    TWeakObjectPtr<AMAChargingStation> ChargingStation;

    // 从充电站获取的交互半径
    float InteractionRadius = 150.f;
};

/**
 * StateTree Task: 自动查找充电站，导航过去并充电
 * 
 * 注意: 自动查找最近的充电站
 * 注意: AcceptanceRadius 从 ChargingStation->InteractionRadius 获取
 */
USTRUCT(meta = (DisplayName = "MA Charge"))
struct MULTIAGENT_API FMASTTask_Charge : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

protected:
    virtual const UStruct* GetInstanceDataType() const override { return FMASTTask_ChargeInstanceData::StaticStruct(); }
    
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
    // 查找最近的充电站，返回充电站指针和投影后的位置
    class AMAChargingStation* FindNearestChargingStation(UWorld* World, const FVector& FromLocation, FVector& OutStationLocation) const;
};
