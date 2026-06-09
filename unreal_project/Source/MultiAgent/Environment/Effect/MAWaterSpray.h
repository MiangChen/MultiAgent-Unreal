// MAWaterSpray.h
// 水喷射特效管理类

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MAWaterSpray.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/** 水喷射模式 */
UENUM(BlueprintType)
enum class EMAWaterSprayMode : uint8
{
    /** 抛物线（受重力影响），用于灭火等慢速喷射场景 */
    Gravity     UMETA(DisplayName = "Gravity (Parabolic)"),

    /** 直线（无重力），用于高压水柱场景 */
    StraightJet UMETA(DisplayName = "Straight Jet (No Gravity)")
};

/**
 * 水喷射特效
 *
 * 使用 Niagara 系统渲染水喷射效果，支持动态调整方向和参数。
 *
 * 两种模式共享同一组用户参数 (SprayDirection / SpraySpeed / SprayWidth)，
 * 在 Niagara 资产层面通过是否启用重力来区分形态：
 * - Gravity:     /Game/VisualEffect/WaterSpray/Particle/P_FountainNoParameter_Converted
 * - StraightJet: /Game/VisualEffect/WaterSpray/Particle/P_WaterJet_Straight
 */
UCLASS()
class MULTIAGENT_API AMAWaterSpray : public AActor
{
    GENERATED_BODY()

public:
    AMAWaterSpray();

    /** 开始喷射，根据当前 Mode 选择 Niagara 资产并瞄准目标 */
    UFUNCTION(BlueprintCallable, Category = "WaterSpray")
    void StartSpray(FVector TargetLocation);

    /**
     * 以显式世界方向开始喷射（直线模式专用）。
     *
     * 与 StartSpray 不同，喷射方向不再由"发射点->目标"推导，而是直接采用给定方向，
     * 因此水柱可以平行于某个法向量喷出，而不必指向目标中心。
     * 水柱长度固定为 JetLength（通常等于机器人到作业面的间距），随机器人移动保持不变。
     *
     * @param WorldDirection 世界空间喷射方向（内部会归一化）
     * @param JetLength 水柱长度 (cm)
     */
    UFUNCTION(BlueprintCallable, Category = "WaterSpray")
    void StartSprayDirectional(FVector WorldDirection, float JetLength);

    /** 停止喷射 */
    UFUNCTION(BlueprintCallable, Category = "WaterSpray")
    void StopSpray();

    /** 更新目标位置 */
    UFUNCTION(BlueprintCallable, Category = "WaterSpray")
    void UpdateTarget(FVector NewTargetLocation);

    /**
     * 更新显式喷射方向（直线模式，配合 StartSprayDirectional 使用）。
     * 机器人移动 / 朝向变化时调用，保持水柱平行于给定法向量。
     */
    UFUNCTION(BlueprintCallable, Category = "WaterSpray")
    void UpdateDirection(FVector NewWorldDirection);

    /** 设置喷射参数 */
    UFUNCTION(BlueprintCallable, Category = "WaterSpray")
    void SetSprayParameters(float InSpraySpeed, float InSprayWidth);

    /** 设置喷射模式（必须在 StartSpray 之前调用） */
    UFUNCTION(BlueprintCallable, Category = "WaterSpray")
    void SetSprayMode(EMAWaterSprayMode InMode) { Mode = InMode; }

    /** 获取当前模式 */
    UFUNCTION(BlueprintPure, Category = "WaterSpray")
    EMAWaterSprayMode GetSprayMode() const { return Mode; }

    /** 是否正在喷射 */
    UFUNCTION(BlueprintPure, Category = "WaterSpray")
    bool IsSpraying() const { return bIsSpraying; }

protected:
    virtual void BeginPlay() override;

    /** Niagara 特效组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UNiagaraComponent> SprayEffect;

    /** 喷射模式（重力 / 直线） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    EMAWaterSprayMode Mode = EMAWaterSprayMode::Gravity;

    /** 重力模式（抛物线）使用的 Niagara 资产路径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString GravityEffectPath = TEXT("/Game/VisualEffect/WaterSpray/Particle/P_FountainNoParameter_Converted.P_FountainNoParameter_Converted");

    /** 直线模式（无重力）使用的 Niagara 资产路径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString StraightEffectPath = TEXT("/Game/VisualEffect/WaterSpray/Particle/P_WaterJet_Straight.P_WaterJet_Straight");

    /** 喷射初速度 (cm/s)，决定射程 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float SpraySpeed = 1000.f;

    /** 水柱宽度/粒子大小 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float SprayWidth = 10.f;

    /**
     * 直线模式下粒子寿命的下限（秒）。
     *
     * 直线模式会用 距离/速度 计算粒子寿命，使水柱长度刚好等于发射点到目标的距离。
     * 当目标距离非常近、计算结果过小时，用此下限兜底，避免水柱完全看不见。
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float MinStraightLifetime = 0.05f;

private:
    bool bIsSpraying = false;
    FVector CurrentTargetLocation;

    /** 是否处于显式方向模式（由 StartSprayDirectional 启用） */
    bool bUseExplicitDirection = false;

    /** 显式喷射方向（世界空间，已归一化） */
    FVector ExplicitDirection = FVector::ForwardVector;

    /** 显式方向模式下的水柱长度 (cm) */
    float ExplicitJetLength = 0.f;

    /** 根据当前模式获取要加载的 Niagara 资产路径 */
    const FString& GetActiveEffectPath() const;

    /** 根据当前模式计算发射方向单位向量 */
    FVector ComputeAimDirection(const FVector& StartPos, const FVector& TargetPos) const;

    /** 更新 Niagara 用户参数 */
    void UpdateSprayParameters();
};
