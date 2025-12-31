// MAGoalActor.h
// Goal 可视化 Actor - 用于显示和选择 Goal
// 
// 用于在 Edit Mode 中显示 Goal，支持选择和编辑
// Requirements: 15.2, 15.3, 15.4, 15.5, 15.6

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MAGoalActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UBillboardComponent;
class UTextRenderComponent;

/**
 * Goal 可视化 Actor
 * 
 * 用于在场景中显示 Goal，支持选择和编辑
 * 使用与 POI 不同的视觉样式以区分
 * Requirements: 15.2, 15.3, 15.4, 15.5, 15.6
 */
UCLASS()
class MULTIAGENT_API AMAGoalActor : public AActor
{
    GENERATED_BODY()

public:
    AMAGoalActor();

    //=========================================================================
    // 组件
    //=========================================================================
    
    /** 根组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootSceneComponent;

    /** 可视化 Mesh - 显示 Goal 标记 (使用星形或旗帜形状) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* VisualMesh;

    /** 碰撞球体 - 用于选择检测 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* CollisionSphere;

    /** 文字渲染组件 - 显示 Goal 标签 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UTextRenderComponent* LabelText;

    //=========================================================================
    // 公共接口
    //=========================================================================
    
    /**
     * 设置关联的 Node ID
     * @param InNodeId Node ID
     */
    UFUNCTION(BlueprintCallable, Category = "Goal")
    void SetNodeId(const FString& InNodeId) { NodeId = InNodeId; }

    /**
     * 获取关联的 Node ID
     * @return Node ID
     */
    UFUNCTION(BlueprintPure, Category = "Goal")
    FString GetNodeId() const { return NodeId; }

    /**
     * 设置高亮状态
     * @param bHighlight 是否高亮
     */
    UFUNCTION(BlueprintCallable, Category = "Goal")
    void SetHighlighted(bool bHighlight);

    /**
     * 是否被高亮
     * @return 高亮状态
     */
    UFUNCTION(BlueprintPure, Category = "Goal")
    bool IsHighlighted() const { return bIsHighlighted; }

    /**
     * 设置 Goal 描述
     * @param InDescription 描述文本
     */
    UFUNCTION(BlueprintCallable, Category = "Goal")
    void SetDescription(const FString& InDescription);

    /**
     * 获取 Goal 描述
     * @return 描述文本
     */
    UFUNCTION(BlueprintPure, Category = "Goal")
    FString GetDescription() const { return Description; }

    /**
     * 设置 Goal 标签
     * @param InLabel 标签文本
     */
    UFUNCTION(BlueprintCallable, Category = "Goal")
    void SetLabel(const FString& InLabel);

    /**
     * 获取 Goal 标签
     * @return 标签文本
     */
    UFUNCTION(BlueprintPure, Category = "Goal")
    FString GetLabel() const { return Label; }

protected:
    virtual void BeginPlay() override;

    //=========================================================================
    // 配置
    //=========================================================================
    
    /** 默认颜色 - 与 POI 不同以区分 (红色表示目标) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor DefaultColor = FLinearColor(1.0f, 0.3f, 0.3f, 1.0f);  // 红色

    /** 高亮颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor HighlightColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);  // 黄色

    /** 碰撞球体半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CollisionRadius = 60.0f;

    /** Mesh 缩放 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    float MeshScale = 0.8f;

private:
    /** 关联的 Node ID */
    FString NodeId;

    /** Goal 描述 */
    FString Description;

    /** Goal 标签 */
    FString Label;

    /** 是否被高亮 */
    bool bIsHighlighted = false;

    /** 动态材质实例 */
    UPROPERTY()
    UMaterialInstanceDynamic* DynamicMaterial;

    /** 更新材质颜色 */
    void UpdateMaterialColor();
};
