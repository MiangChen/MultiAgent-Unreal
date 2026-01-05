// MAGoalActor.cpp
// Goal 可视化 Actor 实现

#include "MAGoalActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AMAGoalActor::AMAGoalActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建根组件
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    SetRootComponent(RootSceneComponent);

    // 创建碰撞球体 - 用于选择检测
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->SetupAttachment(RootSceneComponent);
    CollisionSphere->SetSphereRadius(CollisionRadius);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    CollisionSphere->SetGenerateOverlapEvents(true);

    // 创建可视化 Mesh (使用锥体表示目标)
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(RootSceneComponent);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeScale3D(FVector(MeshScale, MeshScale, MeshScale * 1.5f));
    VisualMesh->SetRelativeRotation(FRotator(180.0f, 0.0f, 0.0f));  // 倒置锥体指向下方

    // 创建文字渲染组件
    LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
    LabelText->SetupAttachment(RootSceneComponent);
    LabelText->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));  // 在 Mesh 上方
    LabelText->SetHorizontalAlignment(EHTA_Center);
    LabelText->SetVerticalAlignment(EVRTA_TextCenter);
    LabelText->SetWorldSize(30.0f);
    LabelText->SetTextRenderColor(FColor::White);
    LabelText->SetText(FText::FromString(TEXT("Goal")));
}

void AMAGoalActor::BeginPlay()
{
    Super::BeginPlay();

    // 加载锥体 Mesh
    UStaticMesh* ConeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone"));
    if (ConeMesh && VisualMesh)
    {
        VisualMesh->SetStaticMesh(ConeMesh);
    }

    // 创建动态材质
    UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
    if (BaseMaterial)
    {
        DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        if (DynamicMaterial && VisualMesh)
        {
            DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), DefaultColor);
            VisualMesh->SetMaterial(0, DynamicMaterial);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AMAGoalActor::BeginPlay: Goal Actor initialized at %s"), 
        *GetActorLocation().ToString());
}

void AMAGoalActor::SetHighlighted(bool bHighlight)
{
    if (bIsHighlighted == bHighlight)
    {
        return;
    }

    bIsHighlighted = bHighlight;
    UpdateMaterialColor();

    UE_LOG(LogTemp, Log, TEXT("AMAGoalActor::SetHighlighted(%s) for Goal %s"), 
        bHighlight ? TEXT("true") : TEXT("false"), *NodeId);
}

void AMAGoalActor::SetDescription(const FString& InDescription)
{
    Description = InDescription;
}

void AMAGoalActor::SetLabel(const FString& InLabel)
{
    Label = InLabel;
    
    if (LabelText)
    {
        LabelText->SetText(FText::FromString(InLabel));
    }
}

void AMAGoalActor::UpdateMaterialColor()
{
    if (!DynamicMaterial)
    {
        return;
    }

    FLinearColor Color = bIsHighlighted ? HighlightColor : DefaultColor;
    DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);

    // 更新文字颜色
    if (LabelText)
    {
        FColor TextColor = bIsHighlighted ? FColor::Yellow : FColor::White;
        LabelText->SetTextRenderColor(TextColor);
    }
}
