// MAWaterSpray.h
// 水喷射特效管理类

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MAWaterSpray.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 水喷射特效
 * 
 * 使用 Niagara 系统渲染水喷射效果，支持动态调整方向和参数。
 * 特效资源: /Game/VisualEffect/WaterSpray/Particle/P_FountainNoParameter_Converted
 * 
 * 物理模型：
 * - 喷水遵循抛物线轨迹
 * - 根据发射点、目标点、初速度自动计算发射角度
 * - SpraySpeed 控制初速度大小，决定射程
 * - SprayWidth 控制粒子大小/水柱宽度
 */
UCLASS()
class MULTIAGENT_API AMAWaterSpray : public AActor
{
    GENERATED_BODY()

public:
    AMAWaterSpray();

    /** 开始喷射，自动计算抛物线发射方向指向目标 */
    UFUNCTION(BlueprintCallable, Category = "WaterSpray")
    void StartSpray(FVector TargetLocation);

    /** 停止喷射 */
    UFUNCTION(BlueprintCallable, Category = "WaterSpray")
    void StopSpray();

    /** 更新目标位置 */
    UFUNCTION(BlueprintCallable, Category = "WaterSpray")
    void UpdateTarget(FVector NewTargetLocation);

    /** 设置喷射参数 */
    UFUNCTION(BlueprintCallable, Category = "WaterSpray")
    void SetSprayParameters(float InSpraySpeed, float InSprayWidth);

    /** 是否正在喷射 */
    UFUNCTION(BlueprintPure, Category = "WaterSpray")
    bool IsSpraying() const { return bIsSpraying; }

protected:
    virtual void BeginPlay() override;

    /** Niagara 特效组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UNiagaraComponent> SprayEffect;

    /** 特效资源路径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString EffectPath = TEXT("/Game/VisualEffect/WaterSpray/Particle/P_FountainNoParameter_Converted.P_FountainNoParameter_Converted");

    /** 喷射初速度 (cm/s)，决定射程 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float SpraySpeed = 1000.f;

    /** 水柱宽度/粒子大小 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float SprayWidth = 10.f;

private:
    bool bIsSpraying = false;
    FVector CurrentTargetLocation;

    /** 更新 Niagara 参数 */
    void UpdateSprayParameters();
    
    /**
     * 计算抛物线发射方向
     * 
     * 根据发射点、目标点、初速度，使用抛物线物理公式计算发射方向。
     * 选择低弧线解（更自然的喷水效果）。
     * 
     * @param StartPos 发射点位置
     * @param TargetPos 目标点位置
     * @param InitialSpeed 初速度大小 (cm/s)
     * @return 发射方向单位向量
     */
    FVector CalculateProjectileLaunchDirection(const FVector& StartPos, const FVector& TargetPos, float InitialSpeed) const;
};
