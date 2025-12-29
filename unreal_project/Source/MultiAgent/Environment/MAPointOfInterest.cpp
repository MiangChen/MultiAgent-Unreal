// MAPointOfInterest.cpp
// POI (Point of Interest) 实现
// Requirements: 3.1, 3.2

#include "MAPointOfInterest.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"

AMAPointOfInterest::AMAPointOfInterest()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建根组件
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    SetRootComponent(RootSceneComponent);

    // 创建碰撞球体
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->SetupAttachment(RootSceneComponent);
    CollisionSphere->SetSphereRadius(CollisionRadius);
    CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollisionSphere->SetGenerateOverlapEvents(true);

    // 创建 Niagara 组件
    NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
    NiagaraComponent->SetupAttachment(RootSceneComponent);
    NiagaraComponent->SetAutoActivate(true);
}

void AMAPointOfInterest::BeginPlay()
{
    Super::BeginPlay();

    // 尝试加载 Niagara 系统
    if (POINiagaraSystem.IsValid())
    {
        UNiagaraSystem* LoadedSystem = POINiagaraSystem.LoadSynchronous();
        if (LoadedSystem && NiagaraComponent)
        {
            NiagaraComponent->SetAsset(LoadedSystem);
            NiagaraComponent->Activate(true);
        }
    }
    else
    {
        // 如果没有配置 Niagara 系统，使用简单的视觉效果
        // 在实际项目中，应该在编辑器中配置 Niagara 资源
        UE_LOG(LogTemp, Warning, TEXT("MAPointOfInterest: No Niagara system configured, using default visualization"));
    }

    // 设置初始颜色
    UpdateNiagaraColor();
}

void AMAPointOfInterest::SetHighlighted(bool bHighlight)
{
    // Requirements: 4.3
    
    if (bIsHighlighted == bHighlight)
    {
        return;
    }

    bIsHighlighted = bHighlight;
    UpdateNiagaraColor();

    UE_LOG(LogTemp, Log, TEXT("MAPointOfInterest: SetHighlighted(%s)"), bHighlight ? TEXT("true") : TEXT("false"));
}

void AMAPointOfInterest::UpdateNiagaraColor()
{
    if (!NiagaraComponent)
    {
        return;
    }

    // 设置 Niagara 参数来改变颜色
    FLinearColor Color = bIsHighlighted ? HighlightColor : DefaultColor;
    
    // 尝试设置常见的颜色参数名
    NiagaraComponent->SetVariableLinearColor(TEXT("Color"), Color);
    NiagaraComponent->SetVariableLinearColor(TEXT("ParticleColor"), Color);
    NiagaraComponent->SetVariableLinearColor(TEXT("User.Color"), Color);

    // 同时设置碰撞球体的可视化（调试用）
    if (CollisionSphere)
    {
        CollisionSphere->SetHiddenInGame(true);  // 默认隐藏碰撞球体
    }
}
