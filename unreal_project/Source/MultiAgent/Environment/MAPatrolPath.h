// MAPatrolPath.h
// 巡逻路径 - 支持编辑器可视化拖拽编辑

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MAPatrolPath.generated.h"

class USplineComponent;
class UBillboardComponent;

UCLASS()
class MULTIAGENT_API AMAPatrolPath : public AActor
{
    GENERATED_BODY()

public:
    AMAPatrolPath();

    // Spline 组件 - 可在编辑器中拖拽编辑
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Patrol")
    USplineComponent* PathSpline;

    // 是否闭合路径（最后一点连回第一点）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    bool bClosedLoop = true;

    // ========== 可视化设置 ==========
    
    // 是否在运行时显示路径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    bool bShowPathAtRuntime = true;

    // 路径线颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    FColor PathColor = FColor::Cyan;

    // 路径点颜色
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    FColor WaypointColor = FColor::Yellow;

    // 路径点大小
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization", meta = (ClampMin = "10.0", ClampMax = "100.0"))
    float WaypointSize = 30.f;

    // 路径线粗细
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization", meta = (ClampMin = "1.0", ClampMax = "20.0"))
    float PathThickness = 3.f;

    // 是否显示路径点编号
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    bool bShowWaypointNumbers = true;

    // 获取所有路径点（世界坐标）
    UFUNCTION(BlueprintCallable, Category = "Patrol")
    TArray<FVector> GetWaypoints() const;

    // 获取指定索引的路径点
    UFUNCTION(BlueprintCallable, Category = "Patrol")
    FVector GetWaypoint(int32 Index) const;

    // 获取下一个路径点索引
    UFUNCTION(BlueprintCallable, Category = "Patrol")
    int32 GetNextWaypointIndex(int32 CurrentIndex) const;

    // 获取路径点数量
    UFUNCTION(BlueprintCallable, Category = "Patrol")
    int32 GetWaypointCount() const;

    // 是否有效路径
    UFUNCTION(BlueprintCallable, Category = "Patrol")
    bool IsValidPath() const { return GetWaypointCount() >= 2; }

    // 兼容旧接口 - 直接访问 Waypoints
    UPROPERTY()
    TArray<FVector> Waypoints;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    // 从 Spline 更新 Waypoints 数组
    void UpdateWaypointsFromSpline();

    // 绘制运行时可视化
    void DrawPathVisualization();

    // Billboard 用于在编辑器中显示图标
    UPROPERTY()
    UBillboardComponent* Billboard;
};
