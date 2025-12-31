// MAZoneActor.h
// Zone 可视化 Actor - 使用 Spline 组件显示区域边界
// 
// 用于在 Edit Mode 中显示 Zone 的边界曲线，支持选择和编辑
// Requirements: 14.1, 14.2, 14.3, 14.4, 14.5, 14.6

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MAZoneActor.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UBoxComponent;

/**
 * Zone 可视化 Actor
 * 
 * 用于在场景中显示 Zone 的边界曲线，支持选择和编辑
 * Requirements: 14.1, 14.2, 14.3, 14.4, 14.5, 14.6
 */
UCLASS()
class MULTIAGENT_API AMAZoneActor : public AActor
{
    GENERATED_BODY()

public:
    AMAZoneActor();

    //=========================================================================
    // 组件
    //=========================================================================
    
    /** 根组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootSceneComponent;

    /** Spline 组件 - 显示 Zone 边界 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USplineComponent* SplineComponent;

    /** 碰撞盒 - 用于选择检测 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* CollisionBox;

    //=========================================================================
    // 公共接口
    //=========================================================================
    
    /**
     * 设置 Zone 顶点
     * @param Vertices 顶点数组 (世界坐标)
     */
    UFUNCTION(BlueprintCallable, Category = "Zone")
    void SetVertices(const TArray<FVector>& Vertices);

    /**
     * 获取 Zone 顶点
     * @return 顶点数组
     */
    UFUNCTION(BlueprintPure, Category = "Zone")
    TArray<FVector> GetVertices() const { return ZoneVertices; }

    /**
     * 设置关联的 Node ID
     * @param InNodeId Node ID
     */
    UFUNCTION(BlueprintCallable, Category = "Zone")
    void SetNodeId(const FString& InNodeId) { NodeId = InNodeId; }

    /**
     * 获取关联的 Node ID
     * @return Node ID
     */
    UFUNCTION(BlueprintPure, Category = "Zone")
    FString GetNodeId() const { return NodeId; }

    /**
     * 设置高亮状态
     * @param bHighlight 是否高亮
     */
    UFUNCTION(BlueprintCallable, Category = "Zone")
    void SetHighlighted(bool bHighlight);

    /**
     * 是否被高亮
     * @return 高亮状态
     */
    UFUNCTION(BlueprintPure, Category = "Zone")
    bool IsHighlighted() const { return bIsHighlighted; }

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    //=========================================================================
    // 配置
    //=========================================================================
    
    /** 默认颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor DefaultColor = FLinearColor(0.2f, 0.5f, 1.0f, 0.8f);  // 蓝色

    /** 高亮颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor HighlightColor = FLinearColor(1.0f, 0.8f, 0.0f, 1.0f);  // 黄色

    /** Spline 线宽 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    float SplineWidth = 20.0f;

    /** Spline Mesh 资源 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    UStaticMesh* SplineMesh;

private:
    /** 关联的 Node ID */
    FString NodeId;

    /** 是否被高亮 */
    bool bIsHighlighted = false;

    /** Zone 顶点 */
    TArray<FVector> ZoneVertices;

    /** Spline Mesh 组件数组 */
    UPROPERTY()
    TArray<USplineMeshComponent*> SplineMeshComponents;

    /** 动态材质实例 */
    UPROPERTY()
    UMaterialInstanceDynamic* DynamicMaterial;

    /** 更新 Spline 渲染 */
    void UpdateSplineRendering();

    /** 更新碰撞盒 */
    void UpdateCollisionBox();

    /** 清除 Spline Mesh 组件 */
    void ClearSplineMeshComponents();
};
