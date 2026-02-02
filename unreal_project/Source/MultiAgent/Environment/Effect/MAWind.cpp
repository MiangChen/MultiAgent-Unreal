// MAWind.cpp
// 风特效环境对象实现

#include "MAWind.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

AMAWind::AMAWind()
{
    // 设置默认类型
    ObjectType = TEXT("wind");
}

void AMAWind::SpawnEffect()
{
    // 首先尝试加载 Niagara 系统 (PS_WindWisps)
    UNiagaraSystem* NiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, *NiagaraWindEffectPath);
    
    if (NiagaraSystem)
    {
        // 使用 Niagara
        NiagaraWindParticle = NewObject<UNiagaraComponent>(this);
        NiagaraWindParticle->SetupAttachment(RootComponent);
        NiagaraWindParticle->SetAsset(NiagaraSystem);
        NiagaraWindParticle->RegisterComponent();
        
        // 应用缩放
        SetActorScale3D(FVector(EffectScale));
        
        // 尝试设置 Niagara 参数 - 颜色改为白色
        NiagaraWindParticle->SetColorParameter(TEXT("Color"), FLinearColor::White);
        NiagaraWindParticle->SetColorParameter(TEXT("WindColor"), FLinearColor::White);
        NiagaraWindParticle->SetColorParameter(TEXT("RibbonColor"), FLinearColor::White);
        
        // 尝试调整密度/数量参数
        NiagaraWindParticle->SetFloatParameter(TEXT("SpawnRate"), 50.0f);
        NiagaraWindParticle->SetFloatParameter(TEXT("RibbonWidth"), 2.0f);
        
        NiagaraWindParticle->Activate(true);
        
        UE_LOG(LogTemp, Log, TEXT("[MAWind] %s spawned Niagara wind effect at %s (scale=%.2f)"), 
            *ObjectLabel, *GetActorLocation().ToString(), EffectScale);
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[MAWind] Failed to load Niagara effect: %s, trying Cascade fallback"), *NiagaraWindEffectPath);
    
    // 回退到 Cascade 粒子系统
    UParticleSystem* CascadeSystem = LoadObject<UParticleSystem>(nullptr, *CascadeWindEffectPath);
    
    if (CascadeSystem)
    {
        CascadeWindParticle = NewObject<UParticleSystemComponent>(this);
        CascadeWindParticle->SetupAttachment(RootComponent);
        CascadeWindParticle->SetTemplate(CascadeSystem);
        CascadeWindParticle->RegisterComponent();
        
        // 应用缩放
        SetActorScale3D(FVector(EffectScale));
        
        CascadeWindParticle->Activate(true);
        
        UE_LOG(LogTemp, Log, TEXT("[MAWind] %s spawned Cascade wind effect at %s (scale=%.2f)"), 
            *ObjectLabel, *GetActorLocation().ToString(), EffectScale);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[MAWind] %s failed to load any wind effect"), *ObjectLabel);
    }
}
