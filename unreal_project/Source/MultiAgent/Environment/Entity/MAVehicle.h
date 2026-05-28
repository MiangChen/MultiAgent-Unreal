// MAVehicle.h
// 车辆环境对象 - 可导航的车辆实体
// 继承 ACharacter 以支持 NavMesh 导航

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../IMAEnvironmentObject.h"
#include "MAVehicle.generated.h"

class UMANavigationService;
class UStaticMeshComponent;
class AMAFire;
struct FMAEnvironmentObjectConfig;

/**
 * 车辆环境对象
 * 
 * 继承 ACharacter，实现 IMAEnvironmentObject 接口。
 * 支持通过配置选择车辆模型和车身颜色。
 * 使用 NavMesh 进行导航。
 * 
 * 配置示例:
 * {
 *     "label": "Car_1",
 *     "type": "vehicle",
 *     "position": [8000, 9500, 0],
 *     "rotation": [0, 180, 0],
 *     "features": {
 *         "subtype": "sedan",
 *         "color": "red"
 *     }
 * }
 */
UCLASS()
class MULTIAGENT_API AMAVehicle : public ACharacter, public IMAEnvironmentObject
{
    GENERATED_BODY()

public:
    AMAVehicle();

    //=========================================================================
    // IMAEnvironmentObject 接口实现
    //=========================================================================

    virtual FString GetObjectLabel() const override;
    virtual FString GetObjectType() const override;
    virtual const TMap<FString, FString>& GetObjectFeatures() const override;

    //=========================================================================
    // 配置方法
    //=========================================================================

    /** 根据配置初始化车辆 */
    UFUNCTION(BlueprintCallable, Category = "Vehicle")
    void Configure(const FMAEnvironmentObjectConfig& Config);

    /** 设置车辆模型 */
    UFUNCTION(BlueprintCallable, Category = "Vehicle")
    void SetVehicleMesh(const FString& Subtype);

    /** 设置车身颜色 */
    UFUNCTION(BlueprintCallable, Category = "Vehicle")
    void SetBodyColor(const FLinearColor& Color);

    //=========================================================================
    // 导航接口
    //=========================================================================

    /** 导航到目标位置 */
    UFUNCTION(BlueprintCallable, Category = "Vehicle")
    bool NavigateTo(FVector Destination, float AcceptanceRadius = 200.f);

    /** 取消导航 */
    UFUNCTION(BlueprintCallable, Category = "Vehicle")
    void CancelNavigation();

    /** 是否正在导航 */
    UFUNCTION(BlueprintPure, Category = "Vehicle")
    bool IsNavigating() const;

    /** 获取导航服务组件 */
    UMANavigationService* GetNavigationService() const { return NavigationService; }

    /** 获取网格组件 */
    UStaticMeshComponent* GetVehicleMesh() const { return VehicleMeshComponent; }

    //=========================================================================
    // 附着火焰管理 (IMAEnvironmentObject 接口)
    //=========================================================================

    /** 设置附着的火焰对象 */
    virtual void SetAttachedFire(AMAFire* Fire) override;
    
    /** 获取附着的火焰对象 */
    virtual AMAFire* GetAttachedFire() const override;

    //=========================================================================
    // 巡逻接口
    //=========================================================================

    /** 启动巡逻 */
    UFUNCTION(BlueprintCallable, Category = "Vehicle|Patrol")
    void StartPatrol();

    /** 停止巡逻 */
    UFUNCTION(BlueprintCallable, Category = "Vehicle|Patrol")
    void StopPatrol();

    /** 是否正在巡逻 */
    UFUNCTION(BlueprintPure, Category = "Vehicle|Patrol")
    bool IsPatrolling() const { return bIsPatrolling; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    //=========================================================================
    // 转向参数
    //=========================================================================

    /** 最小转向速率（高速时）度/秒 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Steering")
    float MinTurnRate = 50.f;

    /** 最大转向速率（低速时）度/秒 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle|Steering")
    float MaxTurnRate = 100.f;

    //=========================================================================
    // 环境对象属性
    //=========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FString ObjectLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FString ObjectType = TEXT("vehicle");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    TMap<FString, FString> Features;

    //=========================================================================
    // 组件
    //=========================================================================

    /** 车辆网格组件 (附加到 Capsule) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> VehicleMeshComponent;

    /** 导航服务组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UMANavigationService> NavigationService;

    /** 附着的火焰对象 */
    UPROPERTY()
    TWeakObjectPtr<AMAFire> AttachedFire;

    //=========================================================================
    // 巡逻相关
    //=========================================================================

    /** 巡逻路径点 */
    UPROPERTY()
    TArray<FVector> PatrolWaypoints;

    /** 是否循环巡逻 */
    bool bPatrolLoop = true;

    /** 到达点后等待时间 */
    float PatrolWaitTime = 2.0f;

    /** 当前目标路径点索引 */
    int32 CurrentWaypointIndex = 0;

    /** 是否正在巡逻 */
    bool bIsPatrolling = false;

    /** 巡逻等待定时器 */
    FTimerHandle PatrolWaitTimerHandle;

private:
    /** 导航完成回调 */
    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);

    /** 移动到下一个巡逻点 */
    void MoveToNextWaypoint();

    /** 更新车辆转向（平滑转弯） */
    void UpdateVehicleSteering(float DeltaTime);

    /** 根据 Mesh 自动调整胶囊体尺寸和位置 */
    void AutoFitCapsuleToMesh();

    /** 解析颜色字符串 */
    static FLinearColor ParseColorString(const FString& ColorStr);

    /** 获取车辆模型路径 */
    static FString GetVehicleMeshPath(const FString& Subtype);
};
