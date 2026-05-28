// MABoat.h
// 船只环境对象 - 使用简单直线移动的水面船只
// 通过 Tick + SetActorLocation 实现移动，保持水面高度

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../IMAEnvironmentObject.h"
#include "MABoat.generated.h"

class UStaticMeshComponent;
class AMAFire;
struct FMAEnvironmentObjectConfig;

/**
 * 船只环境对象
 * 
 * 继承 ACharacter，实现 IMAEnvironmentObject 接口。
 * 支持通过配置选择船只模型，支持水面导航。
 * 
 * 配置示例:
 * {
 *     "label": "Boat_1",
 *     "type": "boat",
 *     "position": [5000, 3000, 0],
 *     "rotation": [0, 90, 0],
 *     "features": {
 *         "subtype": "lifeboat"
 *     }
 * }
 */
UCLASS()
class MULTIAGENT_API AMABoat : public ACharacter, public IMAEnvironmentObject
{
    GENERATED_BODY()

public:
    AMABoat();

    //=========================================================================
    // IMAEnvironmentObject 接口实现
    //=========================================================================

    virtual FString GetObjectLabel() const override;
    virtual FString GetObjectType() const override;
    virtual const TMap<FString, FString>& GetObjectFeatures() const override;

    //=========================================================================
    // 配置方法
    //=========================================================================

    /** 根据配置初始化船只 */
    UFUNCTION(BlueprintCallable, Category = "Boat")
    void Configure(const FMAEnvironmentObjectConfig& Config);

    /** 设置船只模型 */
    UFUNCTION(BlueprintCallable, Category = "Boat")
    void SetBoatMesh(const FString& Subtype);

    //=========================================================================
    // 导航接口
    //=========================================================================

    /** 导航到目标位置 */
    UFUNCTION(BlueprintCallable, Category = "Boat")
    bool NavigateTo(FVector Destination, float AcceptanceRadius = 200.f);

    /** 取消导航 */
    UFUNCTION(BlueprintCallable, Category = "Boat")
    void CancelNavigation();

    /** 是否正在导航 */
    UFUNCTION(BlueprintPure, Category = "Boat")
    bool IsNavigating() const;

    /** 获取网格组件 */
    UStaticMeshComponent* GetBoatMesh() const { return BoatMeshComponent; }

    /** 获取水面高度 */
    UFUNCTION(BlueprintPure, Category = "Boat")
    float GetWaterLevel() const { return WaterLevel; }

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
    UFUNCTION(BlueprintCallable, Category = "Boat|Patrol")
    void StartPatrol();

    /** 停止巡逻 */
    UFUNCTION(BlueprintCallable, Category = "Boat|Patrol")
    void StopPatrol();

    /** 是否正在巡逻 */
    UFUNCTION(BlueprintPure, Category = "Boat|Patrol")
    bool IsPatrolling() const { return bIsPatrolling; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    //=========================================================================
    // 环境对象属性
    //=========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FString ObjectLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FString ObjectType = TEXT("boat");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    TMap<FString, FString> Features;

    //=========================================================================
    // 组件
    //=========================================================================

    /** 船只网格组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> BoatMeshComponent;

    /** 水面高度 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
    float WaterLevel = 0.f;
    
    /** 船体吃水深度（船底在水面下的深度，正值表示下沉） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    float WaterlineDraft = 20.f;

    /** 移动速度 cm/s */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    float MoveSpeed = 300.f;

    /** 转向速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
    float TurnSpeed = 2.f;

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

    /** 当前移动目标 */
    FVector TargetLocation = FVector::ZeroVector;

    /** 到达判定半径 */
    float AcceptanceRadius = 150.f;

    /** 是否正在移动 */
    bool bIsMoving = false;

private:
    /** 移动完成回调 */
    void OnMoveCompleted(bool bSuccess);

    /** 移动到下一个巡逻点 */
    void MoveToNextWaypoint();

    /** 根据 Mesh 自动调整胶囊体尺寸和位置 */
    void AutoFitCapsuleToMesh();

    /** 获取船只模型路径 */
    static FString GetBoatMeshPath(const FString& Subtype);
};
