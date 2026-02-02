// MASmoke.cpp
// 烟雾特效环境对象实现

#include "MASmoke.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

AMASmoke::AMASmoke()
{
    // 设置默认类型
    ObjectType = TEXT("smoke");
}

void AMASmoke::SpawnEffect()
{
    // VisualEffect/Wind 资产包使用 Cascade 粒子系统
    UParticleSystem* SmokeSystem = LoadObject<UParticleSystem>(nullptr, *SmokeEffectPath);
    
    if (!SmokeSystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MASmoke] Failed to load Cascade effect: %s, trying fallback"), *SmokeEffectPath);
        SmokeSystem = LoadObject<UParticleSystem>(nullptr, *FallbackSmokeEffectPath);
    }

    if (SmokeSystem)
    {
        UParticleSystemComponent* PSC = NewObject<UParticleSystemComponent>(this);
        PSC->SetupAttachment(RootComponent);
        PSC->SetTemplate(SmokeSystem);
        PSC->RegisterComponent();
        
        // 应用缩放到 Actor
        SetActorScale3D(FVector(EffectScale));
        
        PSC->Activate(true);
        
        // 保存组件引用
        SmokeParticle = PSC;
        
        UE_LOG(LogTemp, Log, TEXT("[MASmoke] %s spawned smoke effect at %s (scale=%.2f)"), 
            *ObjectLabel, *GetActorLocation().ToString(), EffectScale);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[MASmoke] %s failed to load any smoke effect"), *ObjectLabel);
    }
}
