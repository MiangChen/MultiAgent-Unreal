// MAPointOfInterest.cpp
// POI (Point of Interest) 实现

#include "MAPointOfInterest.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AMAPointOfInterest::AMAPointOfInterest()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建根组件
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    SetRootComponent(RootSceneComponent);

    // 创建碰撞球体 - 用于选择检测
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->SetupAttachment(RootSceneComponent);
    CollisionSphere->SetSphereRadius(CollisionRadius);
    // 使用 Visibility 通道以便射线检测可以命中
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    CollisionSphere->SetGenerateOverlapEvents(true);

    // 创建 Niagara 组件
    NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
    NiagaraComponent->SetupAttachment(RootSceneComponent);
    NiagaraComponent->SetAutoActivate(false);  // 默认不激活，等待 BeginPlay 中设置
    
    // 创建可视化球体组件作为备选方案
    VisualSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualSphere"));
    VisualSphere->SetupAttachment(RootSceneComponent);
    VisualSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualSphere->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));  // 缩小球体
}

void AMAPointOfInterest::BeginPlay()
{
    Super::BeginPlay();

    // 尝试加载 Niagara 系统
    bool bNiagaraLoaded = false;
    if (POINiagaraSystem.IsValid())
    {
        UNiagaraSystem* LoadedSystem = POINiagaraSystem.LoadSynchronous();
        if (LoadedSystem && NiagaraComponent)
        {
            NiagaraComponent->SetAsset(LoadedSystem);
            NiagaraComponent->Activate(true);
            bNiagaraLoaded = true;
        }
    }
    
    // 如果没有 Niagara 系统，使用简单的球体作为可视化
    if (!bNiagaraLoaded && VisualSphere)
    {
        // 加载默认球体 Mesh
        UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere"));
        if (SphereMesh)
        {
            VisualSphere->SetStaticMesh(SphereMesh);
        }
        
        // 创建动态材质
        UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
        if (BaseMaterial)
        {
            UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            if (DynMaterial)
            {
                DynMaterial->SetVectorParameterValue(TEXT("Color"), DefaultColor);
                VisualSphere->SetMaterial(0, DynMaterial);
            }
        }
        
        VisualSphere->SetVisibility(true);
        UE_LOG(LogTemp, Log, TEXT("MAPointOfInterest: Using sphere visualization (no Niagara system configured)"));
    }
    else if (VisualSphere)
    {
        // 有 Niagara 系统时隐藏球体
        VisualSphere->SetVisibility(false);
    }

    // 设置初始颜色
    UpdateNiagaraColor();
}

void AMAPointOfInterest::SetHighlighted(bool bHighlight)
{
    
    if (bIsHighlighted == bHighlight)
    {
        return;
    }

    bIsHighlighted = bHighlight;
    UpdateNiagaraColor();
    
    // 同时设置 Custom Depth 用于轮廓高亮效果
    if (VisualSphere)
    {
        VisualSphere->SetRenderCustomDepth(bHighlight);
        if (bHighlight)
        {
            VisualSphere->SetCustomDepthStencilValue(2);
        }
    }
    
    // 调整球体大小以增强视觉反馈
    if (VisualSphere && VisualSphere->IsVisible())
    {
        FVector Scale = bHighlight ? FVector(0.8f, 0.8f, 0.8f) : FVector(0.5f, 0.5f, 0.5f);
        VisualSphere->SetRelativeScale3D(Scale);
    }

    UE_LOG(LogTemp, Log, TEXT("MAPointOfInterest: SetHighlighted(%s)"), bHighlight ? TEXT("true") : TEXT("false"));
}

void AMAPointOfInterest::UpdateNiagaraColor()
{
    FLinearColor Color = bIsHighlighted ? HighlightColor : DefaultColor;
    
    // 更新 Niagara 颜色参数
    if (NiagaraComponent && NiagaraComponent->IsActive())
    {
        // 尝试设置常见的颜色参数名
        NiagaraComponent->SetVariableLinearColor(TEXT("Color"), Color);
        NiagaraComponent->SetVariableLinearColor(TEXT("ParticleColor"), Color);
        NiagaraComponent->SetVariableLinearColor(TEXT("User.Color"), Color);
        
        // 高亮时增加粒子强度
        if (bIsHighlighted)
        {
            NiagaraComponent->SetVariableFloat(TEXT("SpawnRate"), 50.0f);
            NiagaraComponent->SetVariableFloat(TEXT("ParticleSize"), 20.0f);
        }
        else
        {
            NiagaraComponent->SetVariableFloat(TEXT("SpawnRate"), 20.0f);
            NiagaraComponent->SetVariableFloat(TEXT("ParticleSize"), 10.0f);
        }
    }
    
    // 更新球体材质颜色
    if (VisualSphere && VisualSphere->IsVisible())
    {
        UMaterialInstanceDynamic* DynMaterial = Cast<UMaterialInstanceDynamic>(VisualSphere->GetMaterial(0));
        if (!DynMaterial)
        {
            // 创建动态材质
            UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
            if (BaseMaterial)
            {
                DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
                VisualSphere->SetMaterial(0, DynMaterial);
            }
        }
        
        if (DynMaterial)
        {
            // 设置基础颜色
            DynMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
            
            // 高亮时使用更强的自发光效果
            float EmissiveMultiplier = bIsHighlighted ? 5.0f : 2.0f;
            DynMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), Color * EmissiveMultiplier);
        }
    }

    // 同时设置碰撞球体的可视化（调试用）
    if (CollisionSphere)
    {
        CollisionSphere->SetHiddenInGame(true);  // 默认隐藏碰撞球体
    }
}
