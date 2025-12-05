// MACoverageArea.h
// 覆盖作业区域 - 定义机器人需要覆盖扫描的多边形区域

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MACoverageArea.generated.h"

class USplineComponent;
class UBillboardComponent;

UCLASS()
class MULTIAGENT_API AMACoverageArea : public AActor
{
    GENERATED_BODY()

public:
    AMACoverageArea();

    // Spline 组件 - 定义区域边界（闭合多边形）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coverage")
    USplineComponent* BoundarySpline;

    // 默认扫描半径（用于可视化预览，实际使用时由机器人或 Task 提供）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coverage")
    float DefaultScanRadius = 200.f;

    // 路径重叠因子（0.5-1.0，越大重叠越多）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coverage", meta = (ClampMin = "0.5", ClampMax = "1.0"))
    float OverlapFactor = 0.8f;

    // ========== 可视化设置 ==========
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    bool bShowAreaAtRuntime = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    FColor BoundaryColor = FColor::Orange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    FColor PathColor = FColor::Green;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
    float LineThickness = 2.f;

    // ========== 功能接口 ==========

    // 生成覆盖路径点（割草机模式）
    // @param InScanRadius 机器人扫描半径，0 表示使用默认值
    UFUNCTION(BlueprintCallable, Category = "Coverage")
    TArray<FVector> GenerateCoveragePath(float InScanRadius = 0.f) const;

    // 获取边界点（世界坐标）
    UFUNCTION(BlueprintCallable, Category = "Coverage")
    TArray<FVector> GetBoundaryPoints() const;

    // 获取区域包围盒
    UFUNCTION(BlueprintCallable, Category = "Coverage")
    FBox GetAreaBounds() const;

    // 检查点是否在区域内
    UFUNCTION(BlueprintCallable, Category = "Coverage")
    bool IsPointInArea(const FVector& Point) const;

    // 获取路径点数量（使用默认半径）
    UFUNCTION(BlueprintCallable, Category = "Coverage")
    int32 GetCoveragePathCount() const;

    // 设置实际执行路径（由 Task 计算后传入，用于可视化）
    UFUNCTION(BlueprintCallable, Category = "Coverage")
    void SetActivePath(const TArray<FVector>& InPath);

    // 清除实际执行路径
    UFUNCTION(BlueprintCallable, Category = "Coverage")
    void ClearActivePath();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    // 绘制运行时可视化
    void DrawAreaVisualization();

    // 内部路径生成（不使用缓存）
    TArray<FVector> GenerateCoveragePathInternal(float ScanRadius) const;

    // 缓存的覆盖路径
    mutable TArray<FVector> CachedCoveragePath;
    mutable float CachedScanRadius = 0.f;
    mutable bool bPathCacheDirty = true;

    // 实际执行路径（由 Task 设置）
    TArray<FVector> ActivePath;

    UPROPERTY()
    UBillboardComponent* Billboard;
};
