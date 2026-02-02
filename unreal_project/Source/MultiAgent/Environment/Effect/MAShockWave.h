// MAShockWave.h
// 声波特效管理类

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MAShockWave.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 声波特效
 * 
 * 使用 Niagara 系统渲染声波扩散效果，支持动态调整方向和参数。
 * 特效资源: /Game/NS_Shock
 * 
 * 效果说明：
 * - 声波从发射点向目标方向呈锥形扩散
 * - ShockSpeed 控制声波扩散速度
 * - ShockWidth 控制声波锥形角度/宽度
 */
UCLASS()
class MULTIAGENT_API AMAShockWave : public AActor
{
    GENERATED_BODY()

public:
    AMAShockWave();

    /** 开始播放声波特效，指向目标方向 */
    UFUNCTION(BlueprintCallable, Category = "ShockWave")
    void StartShockWave(FVector TargetLocation);

    /** 停止声波特效 */
    UFUNCTION(BlueprintCallable, Category = "ShockWave")
    void StopShockWave();

    /** 更新目标位置 */
    UFUNCTION(BlueprintCallable, Category = "ShockWave")
    void UpdateTarget(FVector NewTargetLocation);

    /** 设置声波参数 */
    UFUNCTION(BlueprintCallable, Category = "ShockWave")
    void SetShockWaveParameters(float InShockSpeed, float InShockWidth, float InShockRate);

    /** 是否正在播放 */
    UFUNCTION(BlueprintPure, Category = "ShockWave")
    bool IsPlaying() const { return bIsPlaying; }

protected:
    virtual void BeginPlay() override;

    /** Niagara 特效组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UNiagaraComponent> ShockWaveEffect;

    /** 特效资源路径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString EffectPath = TEXT("/Game/VisualEffect/ShockWave/NS_Shock.NS_Shock");

    /** 声波扩散速度 (cm/s) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float ShockSpeed = 500.f;

    /** 声波宽度/锥形角度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float ShockWidth = 30.f;

    /** 声波发射频率 (次/秒) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float ShockRate = 5.0f;

private:
    bool bIsPlaying = false;
    FVector CurrentTargetLocation;

    /** 更新 Niagara 参数 */
    void UpdateShockWaveParameters();
};
