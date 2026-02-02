// MAWaterSpray.cpp
// 水喷射特效实现

#include "MAWaterSpray.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"

AMAWaterSpray::AMAWaterSpray()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建根组件
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AMAWaterSpray::BeginPlay()
{
    Super::BeginPlay();
}

void AMAWaterSpray::StartSpray(FVector TargetLocation)
{
    if (bIsSpraying) return;

    CurrentTargetLocation = TargetLocation;

    // 加载 Niagara 特效
    UNiagaraSystem* SpraySystem = LoadObject<UNiagaraSystem>(nullptr, *EffectPath);
    if (!SpraySystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAWaterSpray] Failed to load effect: %s"), *EffectPath);
        // 即使没有特效也标记为喷射中，让技能流程继续
        bIsSpraying = true;
        return;
    }

    // 创建 Niagara 组件
    SprayEffect = NewObject<UNiagaraComponent>(this);
    SprayEffect->SetupAttachment(RootComponent);
    SprayEffect->SetAsset(SpraySystem);
    SprayEffect->RegisterComponent();

    // 设置参数
    UpdateSprayParameters();

    // 激活特效
    SprayEffect->Activate(true);
    bIsSpraying = true;
    
    FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal();
    UE_LOG(LogTemp, Log, TEXT("[MAWaterSpray] Started spray at %s targeting %s, direction=%s"),
        *GetActorLocation().ToString(), *TargetLocation.ToString(), *Direction.ToString());
}

void AMAWaterSpray::StopSpray()
{
    if (!bIsSpraying) return;

    if (SprayEffect)
    {
        SprayEffect->Deactivate();
        SprayEffect->DestroyComponent();
        SprayEffect = nullptr;
    }

    bIsSpraying = false;
    UE_LOG(LogTemp, Log, TEXT("[MAWaterSpray] Stopped spray"));
}

void AMAWaterSpray::UpdateTarget(FVector NewTargetLocation)
{
    CurrentTargetLocation = NewTargetLocation;
    if (bIsSpraying)
    {
        UpdateSprayParameters();
    }
}

void AMAWaterSpray::SetSprayParameters(float InSpraySpeed, float InSprayWidth)
{
    SpraySpeed = InSpraySpeed;
    SprayWidth = InSprayWidth;
    
    UE_LOG(LogTemp, Log, TEXT("[MAWaterSpray] SetSprayParameters: Speed=%.0f, Width=%.0f"),
        SpraySpeed, SprayWidth);
}

FVector AMAWaterSpray::CalculateProjectileLaunchDirection(const FVector& StartPos, const FVector& TargetPos, float InitialSpeed) const
{
    // 计算发射方向（水平发射）
    FVector Direction = (TargetPos - StartPos).GetSafeNormal2D();
    
    float Distance = FVector::Dist(StartPos, TargetPos);
    float HorizontalDistance = FVector::Dist2D(StartPos, TargetPos);
    float HeightDiff = TargetPos.Z - StartPos.Z;
    
    UE_LOG(LogTemp, Log, TEXT("[MAWaterSpray] Direct aim: Distance=%.0f, HorizontalDist=%.0f, HeightDiff=%.0f, Direction=%s"),
        Distance, HorizontalDistance, HeightDiff, *Direction.ToString());
    
    return Direction;
}

void AMAWaterSpray::UpdateSprayParameters()
{
    if (!SprayEffect) return;

    // 计算发射方向（世界空间）
    FVector Direction = CalculateProjectileLaunchDirection(GetActorLocation(), CurrentTargetLocation, SpraySpeed);

    // 设置 Niagara 参数
    // SprayDirection: 发射方向（单位向量，世界空间）
    // SpraySpeed: 初速度大小
    // SprayWidth: 水柱宽度/粒子大小
    
    SprayEffect->SetVectorParameter(FName("SprayDirection"), Direction);
    SprayEffect->SetFloatParameter(FName("SpraySpeed"), SpraySpeed);
    SprayEffect->SetFloatParameter(FName("SprayWidth"), SprayWidth);
    
    // 同时设置组件旋转，使粒子系统的局部坐标系对齐发射方向
    FRotator SprayRotation = Direction.Rotation();
    SprayEffect->SetWorldRotation(SprayRotation);
    
    UE_LOG(LogTemp, Log, TEXT("[MAWaterSpray] Updated parameters: Direction=%s, Speed=%.0f, Width=%.0f"),
        *Direction.ToString(), SpraySpeed, SprayWidth);
}
