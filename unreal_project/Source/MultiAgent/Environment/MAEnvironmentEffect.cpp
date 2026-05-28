// MAEnvironmentEffect.cpp
// 环境特效基类实现

#include "MAEnvironmentEffect.h"
#include "../Core/Config/MAConfigManager.h"

AMAEnvironmentEffect::AMAEnvironmentEffect()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建场景根组件
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

void AMAEnvironmentEffect::BeginPlay()
{
    Super::BeginPlay();

    // 注意: SpawnEffect 现在由子类在 Configure 之后手动调用
    // 或者在 Configure 中调用，确保参数已设置
}

void AMAEnvironmentEffect::Configure(const FMAEnvironmentObjectConfig& Config)
{
    ObjectLabel = Config.Label;
    ObjectType = Config.Type;
    Features = Config.Features;

    // 解析 scale 参数
    if (const FString* ScaleStr = Features.Find(TEXT("scale")))
    {
        EffectScale = FCString::Atof(**ScaleStr);
        if (EffectScale <= 0.f) EffectScale = 1.0f;
    }

    // 解析 intensity 参数
    if (const FString* IntensityStr = Features.Find(TEXT("intensity")))
    {
        EffectIntensity = FCString::Atof(**IntensityStr);
        if (EffectIntensity <= 0.f) EffectIntensity = 1.0f;
    }

    UE_LOG(LogTemp, Log, TEXT("[MAEnvironmentEffect] Configured: Label=%s, Type=%s, Scale=%.2f, Intensity=%.2f"), 
        *ObjectLabel, *ObjectType, EffectScale, EffectIntensity);

    // Configure 完成后生成特效，确保参数已设置
    SpawnEffect();
}

void AMAEnvironmentEffect::SpawnEffect()
{
    // 基类空实现，子类负责生成具体特效
    UE_LOG(LogTemp, Warning, TEXT("[MAEnvironmentEffect] SpawnEffect called on base class - subclass should override"));
}

//=========================================================================
// IMAEnvironmentObject 接口实现
//=========================================================================

FString AMAEnvironmentEffect::GetObjectLabel() const
{
    return ObjectLabel;
}

FString AMAEnvironmentEffect::GetObjectType() const
{
    return ObjectType;
}

const TMap<FString, FString>& AMAEnvironmentEffect::GetObjectFeatures() const
{
    return Features;
}
