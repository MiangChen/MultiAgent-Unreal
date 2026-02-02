// MAFire.cpp
// 火焰特效环境对象实现

#include "MAFire.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

AMAFire::AMAFire()
{
    // 设置默认类型
    ObjectType = TEXT("fire");
}

void AMAFire::SpawnEffect()
{
    // VisualEffect/Fire 资产包使用 Cascade 粒子系统
    UParticleSystem* FireSystem = LoadObject<UParticleSystem>(nullptr, *FireEffectPath);
    
    if (!FireSystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAFire] Failed to load Cascade effect: %s, trying fallback"), *FireEffectPath);
        FireSystem = LoadObject<UParticleSystem>(nullptr, *FallbackFireEffectPath);
    }

    if (FireSystem)
    {
        UParticleSystemComponent* PSC = NewObject<UParticleSystemComponent>(this);
        PSC->SetupAttachment(RootComponent);
        PSC->SetTemplate(FireSystem);
        PSC->RegisterComponent();
        
        // 应用缩放到 Actor
        SetActorScale3D(FVector(EffectScale));
        
        PSC->Activate(true);
        
        // 保存组件引用
        FireParticle = PSC;
        
        UE_LOG(LogTemp, Log, TEXT("[MAFire] %s spawned fire effect at %s (scale=%.2f)"), 
            *ObjectLabel, *GetActorLocation().ToString(), EffectScale);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[MAFire] %s failed to load any fire effect"), *ObjectLabel);
    }
}

void AMAFire::Extinguish()
{
    if (bIsExtinguished) return;
    
    bIsExtinguished = true;
    
    if (FireParticle)
    {
        FireParticle->Deactivate();
        FireParticle->DestroyComponent();
        FireParticle = nullptr;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MAFire] %s extinguished"), *ObjectLabel);
}
