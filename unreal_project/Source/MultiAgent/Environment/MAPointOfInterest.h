// MAPointOfInterest.h
// POI (Point of Interest) - Edit Mode 中的临时标记点
// 
// POI 用于在 Edit Mode 中标记用户感兴趣的位置
// 可用于创建 Goal 或 Zone
//
// Requirements: 3.1, 3.2

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MAPointOfInterest.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USphereComponent;

/**
 * POI (Point of Interest) Actor
 * 
 * Edit Mode 中的临时标记点，包含 Niagara 粒子效果作为视觉标记
 * 
 * Requirements: 3.1, 3.2
 */
UCLASS()
class MULTIAGENT_API AMAPointOfInterest : public AActor
{
    GENERATED_BODY()

public:
    AMAPointOfInterest();

    //=========================================================================
    // 组件
    //=========================================================================
    
    /** 根组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootSceneComponent;

    /** Niagara 粒子组件 - 视觉标记 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UNiagaraComponent* NiagaraComponent;

    /** 碰撞球体 - 用于选择检测 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* CollisionSphere;

    //=========================================================================
    // 公共接口
    //=========================================================================
    
    /**
     * 设置高亮状态
     * Requirements: 4.3
     * 
     * @param bHighlight 是否高亮
     */
    UFUNCTION(BlueprintCallable, Category = "POI")
    void SetHighlighted(bool bHighlight);

    /**
     * 是否被高亮
     * 
     * @return 高亮状态
     */
    UFUNCTION(BlueprintPure, Category = "POI")
    bool IsHighlighted() const { return bIsHighlighted; }

    /**
     * 获取世界坐标
     * 
     * @return 世界坐标
     */
    UFUNCTION(BlueprintPure, Category = "POI")
    FVector GetWorldLocation() const { return GetActorLocation(); }

protected:
    virtual void BeginPlay() override;

    //=========================================================================
    // 配置
    //=========================================================================
    
    /** Niagara 系统资源 (软引用) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    TSoftObjectPtr<UNiagaraSystem> POINiagaraSystem;

    /** 默认颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor DefaultColor = FLinearColor(0.0f, 1.0f, 0.5f, 1.0f);  // 绿色

    /** 高亮颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FLinearColor HighlightColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);  // 黄色

    /** 碰撞球体半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CollisionRadius = 50.0f;

private:
    /** 是否被高亮 */
    bool bIsHighlighted = false;

    /** 更新 Niagara 颜色 */
    void UpdateNiagaraColor();
};
