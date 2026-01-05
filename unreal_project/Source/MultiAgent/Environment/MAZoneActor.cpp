// MAZoneActor.cpp
// Zone 可视化 Actor 实现

#include "MAZoneActor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AMAZoneActor::AMAZoneActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建根组件
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    SetRootComponent(RootSceneComponent);

    // 创建 Spline 组件
    SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
    SplineComponent->SetupAttachment(RootSceneComponent);
    SplineComponent->SetClosedLoop(true);  // Zone 是闭合的
    SplineComponent->ClearSplinePoints();

    // 创建碰撞盒 - 用于选择检测
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionBox->SetupAttachment(RootSceneComponent);
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    CollisionBox->SetGenerateOverlapEvents(true);
    CollisionBox->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));
}

void AMAZoneActor::BeginPlay()
{
    Super::BeginPlay();

    // 加载默认 Spline Mesh (使用圆柱体)
    if (!SplineMesh)
    {
        SplineMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder"));
    }

    // 创建动态材质
    UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
    if (BaseMaterial)
    {
        DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        if (DynamicMaterial)
        {
            DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), DefaultColor);
        }
    }

    // 更新渲染
    UpdateSplineRendering();
}

void AMAZoneActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    // 在编辑器中也更新渲染
    if (ZoneVertices.Num() >= 3)
    {
        UpdateSplineRendering();
    }
}

void AMAZoneActor::SetVertices(const TArray<FVector>& Vertices)
{
    ZoneVertices = Vertices;

    if (ZoneVertices.Num() < 3)
    {
        UE_LOG(LogTemp, Warning, TEXT("AMAZoneActor::SetVertices: Need at least 3 vertices"));
        return;
    }

    // 更新 Spline 点
    SplineComponent->ClearSplinePoints();
    for (int32 i = 0; i < ZoneVertices.Num(); ++i)
    {
        SplineComponent->AddSplinePoint(ZoneVertices[i], ESplineCoordinateSpace::World, true);
    }
    SplineComponent->SetClosedLoop(true);
    SplineComponent->UpdateSpline();

    // 设置 Actor 位置为顶点中心
    FVector Center = FVector::ZeroVector;
    for (const FVector& V : ZoneVertices)
    {
        Center += V;
    }
    Center /= ZoneVertices.Num();
    SetActorLocation(Center);

    // 更新碰撞盒和渲染
    UpdateCollisionBox();
    UpdateSplineRendering();

    UE_LOG(LogTemp, Log, TEXT("AMAZoneActor::SetVertices: Set %d vertices for Zone %s"), 
        ZoneVertices.Num(), *NodeId);
}

void AMAZoneActor::SetHighlighted(bool bHighlight)
{
    if (bIsHighlighted == bHighlight)
    {
        return;
    }

    bIsHighlighted = bHighlight;

    // 更新材质颜色
    if (DynamicMaterial)
    {
        FLinearColor Color = bIsHighlighted ? HighlightColor : DefaultColor;
        DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
    }

    // 更新 Spline Mesh 组件的材质
    for (USplineMeshComponent* MeshComp : SplineMeshComponents)
    {
        if (MeshComp && DynamicMaterial)
        {
            MeshComp->SetMaterial(0, DynamicMaterial);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AMAZoneActor::SetHighlighted(%s) for Zone %s"), 
        bHighlight ? TEXT("true") : TEXT("false"), *NodeId);
}

void AMAZoneActor::UpdateSplineRendering()
{
    // 清除现有的 Spline Mesh 组件
    ClearSplineMeshComponents();

    if (!SplineComponent || SplineComponent->GetNumberOfSplinePoints() < 2)
    {
        return;
    }

    if (!SplineMesh)
    {
        // 如果没有 Mesh，使用 Spline 组件自身的调试渲染
        SplineComponent->SetDrawDebug(true);
        SplineComponent->SetUnselectedSplineSegmentColor(
            bIsHighlighted ? HighlightColor : DefaultColor);
        return;
    }

    // 为每个 Spline 段创建 Spline Mesh 组件
    int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
    int32 NumSegments = SplineComponent->IsClosedLoop() ? NumPoints : NumPoints - 1;

    for (int32 i = 0; i < NumSegments; ++i)
    {
        USplineMeshComponent* SplineMeshComp = NewObject<USplineMeshComponent>(this);
        SplineMeshComp->SetupAttachment(RootSceneComponent);
        SplineMeshComp->SetStaticMesh(SplineMesh);
        SplineMeshComp->SetMobility(EComponentMobility::Movable);
        SplineMeshComp->RegisterComponent();

        // 设置材质
        if (DynamicMaterial)
        {
            SplineMeshComp->SetMaterial(0, DynamicMaterial);
        }

        // 获取起点和终点
        int32 StartIndex = i;
        int32 EndIndex = (i + 1) % NumPoints;

        FVector StartPos, StartTangent, EndPos, EndTangent;
        SplineComponent->GetLocationAndTangentAtSplinePoint(StartIndex, StartPos, StartTangent, ESplineCoordinateSpace::Local);
        SplineComponent->GetLocationAndTangentAtSplinePoint(EndIndex, EndPos, EndTangent, ESplineCoordinateSpace::Local);

        // 设置 Spline Mesh 的起点和终点
        SplineMeshComp->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);

        // 设置缩放 (线宽)
        float Scale = SplineWidth / 100.0f;  // 假设圆柱体直径为 100
        SplineMeshComp->SetStartScale(FVector2D(Scale, Scale));
        SplineMeshComp->SetEndScale(FVector2D(Scale, Scale));

        // 禁用碰撞 (使用 CollisionBox 进行选择)
        SplineMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        SplineMeshComponents.Add(SplineMeshComp);
    }

    UE_LOG(LogTemp, Log, TEXT("AMAZoneActor::UpdateSplineRendering: Created %d spline mesh segments"), 
        SplineMeshComponents.Num());
}

void AMAZoneActor::UpdateCollisionBox()
{
    if (ZoneVertices.Num() < 3 || !CollisionBox)
    {
        return;
    }

    // 计算边界框
    FVector Min = ZoneVertices[0];
    FVector Max = ZoneVertices[0];

    for (const FVector& V : ZoneVertices)
    {
        Min.X = FMath::Min(Min.X, V.X);
        Min.Y = FMath::Min(Min.Y, V.Y);
        Min.Z = FMath::Min(Min.Z, V.Z);
        Max.X = FMath::Max(Max.X, V.X);
        Max.Y = FMath::Max(Max.Y, V.Y);
        Max.Z = FMath::Max(Max.Z, V.Z);
    }

    // 设置碰撞盒大小和位置
    FVector Center = (Min + Max) * 0.5f;
    FVector Extent = (Max - Min) * 0.5f;
    Extent.Z = FMath::Max(Extent.Z, 50.0f);  // 最小高度

    CollisionBox->SetWorldLocation(Center);
    CollisionBox->SetBoxExtent(Extent + FVector(20.0f, 20.0f, 20.0f));  // 稍微扩大一点
}

void AMAZoneActor::ClearSplineMeshComponents()
{
    for (USplineMeshComponent* MeshComp : SplineMeshComponents)
    {
        if (MeshComp)
        {
            MeshComp->DestroyComponent();
        }
    }
    SplineMeshComponents.Empty();
}
