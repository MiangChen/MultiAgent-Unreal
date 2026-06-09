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

    // 加载当前模式对应的 Niagara 资产
    const FString& ActivePath = GetActiveEffectPath();
    UNiagaraSystem* SpraySystem = LoadObject<UNiagaraSystem>(nullptr, *ActivePath);
    if (!SpraySystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAWaterSpray] Failed to load effect: %s"), *ActivePath);
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

    const FVector Direction = ComputeAimDirection(GetActorLocation(), TargetLocation);
    UE_LOG(LogTemp, Log, TEXT("[MAWaterSpray] Started spray (mode=%s) at %s targeting %s, direction=%s"),
        Mode == EMAWaterSprayMode::StraightJet ? TEXT("StraightJet") : TEXT("Gravity"),
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

void AMAWaterSpray::StartSprayDirectional(FVector WorldDirection, float JetLength)
{
    bUseExplicitDirection = true;
    ExplicitDirection = WorldDirection.GetSafeNormal();
    ExplicitJetLength = JetLength;

    // 显式方向喷射只在直线模式下有物理意义（无重力，水柱沿固定方向）
    Mode = EMAWaterSprayMode::StraightJet;

    // 复用 StartSpray 的资产加载/组件创建流程；目标点按方向和长度推导，仅用于内部一致性
    StartSpray(GetActorLocation() + ExplicitDirection * FMath::Max(JetLength, 1.f));
}

void AMAWaterSpray::UpdateDirection(FVector NewWorldDirection)
{
    ExplicitDirection = NewWorldDirection.GetSafeNormal();
    bUseExplicitDirection = true;
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

const FString& AMAWaterSpray::GetActiveEffectPath() const
{
    return Mode == EMAWaterSprayMode::StraightJet ? StraightEffectPath : GravityEffectPath;
}

FVector AMAWaterSpray::ComputeAimDirection(const FVector& StartPos, const FVector& TargetPos) const
{
    // 显式方向模式：直接使用外部给定方向（平行于法向量），不指向目标中心
    if (bUseExplicitDirection)
    {
        return ExplicitDirection;
    }

    // 直线模式：水柱没有重力，直接指向目标，瞄哪打哪
    if (Mode == EMAWaterSprayMode::StraightJet)
    {
        const FVector Direction = (TargetPos - StartPos).GetSafeNormal();
        UE_LOG(LogTemp, Verbose, TEXT("[MAWaterSpray] Straight aim direction=%s"), *Direction.ToString());
        return Direction;
    }

    // 重力模式：粒子受重力下沉，先按水平方向发射，由抛物线落到目标
    const FVector Direction = (TargetPos - StartPos).GetSafeNormal2D();
    const float Distance = FVector::Dist(StartPos, TargetPos);
    const float HorizontalDistance = FVector::Dist2D(StartPos, TargetPos);
    const float HeightDiff = TargetPos.Z - StartPos.Z;

    UE_LOG(LogTemp, Log, TEXT("[MAWaterSpray] Gravity aim: Distance=%.0f, HorizontalDist=%.0f, HeightDiff=%.0f, Direction=%s"),
        Distance, HorizontalDistance, HeightDiff, *Direction.ToString());

    return Direction;
}

void AMAWaterSpray::UpdateSprayParameters()
{
    if (!SprayEffect) return;

    // 计算发射方向（世界空间）
    const FVector Direction = ComputeAimDirection(GetActorLocation(), CurrentTargetLocation);

    // Niagara 用户参数
    // SprayDirection: 发射方向（单位向量，世界空间）
    // SpraySpeed:     初速度大小
    // SprayWidth:     水柱宽度/粒子大小
    // SprayLifetime:  粒子寿命（秒），仅直线模式有效；用于把水柱长度截断到目标距离
    SprayEffect->SetVectorParameter(FName("SprayDirection"), Direction);
    SprayEffect->SetFloatParameter(FName("SpraySpeed"), SpraySpeed);
    SprayEffect->SetFloatParameter(FName("SprayWidth"), SprayWidth);

    if (Mode == EMAWaterSprayMode::StraightJet)
    {
        // 显式方向模式用固定水柱长度；否则用 发射点->目标 的距离
        const float Distance = bUseExplicitDirection
            ? ExplicitJetLength
            : FVector::Dist(GetActorLocation(), CurrentTargetLocation);
        const float Lifetime = SpraySpeed > KINDA_SMALL_NUMBER
            ? FMath::Max(MinStraightLifetime, Distance / SpraySpeed)
            : MinStraightLifetime;
        SprayEffect->SetFloatParameter(FName("SprayLifetime"), Lifetime);

        UE_LOG(LogTemp, Verbose, TEXT("[MAWaterSpray] StraightJet lifetime=%.3fs (dist=%.0f, speed=%.0f)"),
            Lifetime, Distance, SpraySpeed);
    }

    // 同时让组件本身朝向喷射方向，保证局部坐标系下的 Cone/Mesh 等模块朝向正确
    SprayEffect->SetWorldRotation(Direction.Rotation());

    UE_LOG(LogTemp, Log, TEXT("[MAWaterSpray] Updated parameters: Direction=%s, Speed=%.0f, Width=%.0f"),
        *Direction.ToString(), SpraySpeed, SprayWidth);
}
