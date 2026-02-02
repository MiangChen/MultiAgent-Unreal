// MAShockWave.cpp
// 声波特效实现

#include "MAShockWave.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"

AMAShockWave::AMAShockWave()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建根组件
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AMAShockWave::BeginPlay()
{
    Super::BeginPlay();
}

void AMAShockWave::StartShockWave(FVector TargetLocation)
{
    if (bIsPlaying) return;

    CurrentTargetLocation = TargetLocation;

    // 加载 Niagara 特效
    UNiagaraSystem* ShockSystem = LoadObject<UNiagaraSystem>(nullptr, *EffectPath);
    if (!ShockSystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAShockWave] Failed to load effect: %s"), *EffectPath);
        // 即使没有特效也标记为播放中，让技能流程继续
        bIsPlaying = true;
        return;
    }

    // 创建 Niagara 组件
    ShockWaveEffect = NewObject<UNiagaraComponent>(this);
    ShockWaveEffect->SetupAttachment(RootComponent);
    ShockWaveEffect->SetAsset(ShockSystem);
    ShockWaveEffect->RegisterComponent();

    // 设置参数
    UpdateShockWaveParameters();

    // 激活特效
    ShockWaveEffect->Activate(true);
    bIsPlaying = true;
    
    FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal();
    UE_LOG(LogTemp, Log, TEXT("[MAShockWave] Started shock wave at %s targeting %s, direction=%s"),
        *GetActorLocation().ToString(), *TargetLocation.ToString(), *Direction.ToString());
}

void AMAShockWave::StopShockWave()
{
    if (!bIsPlaying) return;

    if (ShockWaveEffect)
    {
        ShockWaveEffect->Deactivate();
        ShockWaveEffect->DestroyComponent();
        ShockWaveEffect = nullptr;
    }

    bIsPlaying = false;
    UE_LOG(LogTemp, Log, TEXT("[MAShockWave] Stopped shock wave"));
}

void AMAShockWave::UpdateTarget(FVector NewTargetLocation)
{
    CurrentTargetLocation = NewTargetLocation;
    if (bIsPlaying)
    {
        UpdateShockWaveParameters();
    }
}

void AMAShockWave::SetShockWaveParameters(float InShockSpeed, float InShockWidth, float InShockRate)
{
    ShockSpeed = InShockSpeed;
    ShockWidth = InShockWidth;
    ShockRate = InShockRate;
    
    UE_LOG(LogTemp, Log, TEXT("[MAShockWave] SetShockWaveParameters: Speed=%.0f, Width=%.0f, Rate=%.1f"),
        ShockSpeed, ShockWidth, ShockRate);
}

void AMAShockWave::UpdateShockWaveParameters()
{
    if (!ShockWaveEffect) return;

    // 计算发射方向（世界空间）
    FVector Direction = (CurrentTargetLocation - GetActorLocation()).GetSafeNormal();
    
    if (Direction.IsNearlyZero())
    {
        Direction = GetActorForwardVector();
    }

    // 设置 Niagara 参数（如果特效支持这些参数）
    ShockWaveEffect->SetVectorParameter(FName("ShockDirection"), Direction);
    ShockWaveEffect->SetFloatParameter(FName("ShockSpeed"), ShockSpeed);
    ShockWaveEffect->SetFloatParameter(FName("ShockWidth"), ShockWidth);
    ShockWaveEffect->SetFloatParameter(FName("ShockRate"), ShockRate);
    
    // 设置组件旋转，使粒子系统的局部坐标系对齐发射方向
    FRotator ShockRotation = Direction.Rotation();
    ShockWaveEffect->SetWorldRotation(ShockRotation);
    
    UE_LOG(LogTemp, Log, TEXT("[MAShockWave] Updated parameters: Direction=%s, Speed=%.0f, Width=%.0f, Rate=%.1f"),
        *Direction.ToString(), ShockSpeed, ShockWidth, ShockRate);
}
