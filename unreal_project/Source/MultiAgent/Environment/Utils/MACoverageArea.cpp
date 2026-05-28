// MACoverageArea.cpp

#include "MACoverageArea.h"
#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"
#include "DrawDebugHelpers.h"

AMACoverageArea::AMACoverageArea()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    // 自动添加 Actor Tag
    Tags.Add(FName("CoverageArea"));

    // 创建 Billboard（编辑器图标）
    Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    RootComponent = Billboard;

    // 创建边界 Spline
    BoundarySpline = CreateDefaultSubobject<USplineComponent>(TEXT("BoundarySpline"));
    BoundarySpline->SetupAttachment(RootComponent);
    BoundarySpline->SetDrawDebug(true);
    BoundarySpline->SetClosedLoop(true);
    BoundarySpline->SetUnselectedSplineSegmentColor(FLinearColor(1.f, 0.5f, 0.f));  // 橙色

    // 默认创建一个矩形区域
    BoundarySpline->ClearSplinePoints(false);
    BoundarySpline->AddSplinePoint(FVector(0.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
    BoundarySpline->AddSplinePoint(FVector(1000.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
    BoundarySpline->AddSplinePoint(FVector(1000.f, 800.f, 0.f), ESplineCoordinateSpace::Local, false);
    BoundarySpline->AddSplinePoint(FVector(0.f, 800.f, 0.f), ESplineCoordinateSpace::Local, false);

    // 设置为线性（直线边界）
    for (int32 i = 0; i < BoundarySpline->GetNumberOfSplinePoints(); ++i)
    {
        BoundarySpline->SetSplinePointType(i, ESplinePointType::Linear, false);
    }
    BoundarySpline->UpdateSpline();
}

void AMACoverageArea::BeginPlay()
{
    Super::BeginPlay();
    bPathCacheDirty = true;
    SetActorTickEnabled(bShowAreaAtRuntime);
}

void AMACoverageArea::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (bShowAreaAtRuntime)
    {
        DrawAreaVisualization();
    }
}

void AMACoverageArea::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    bPathCacheDirty = true;
}

#if WITH_EDITOR
void AMACoverageArea::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    bPathCacheDirty = true;
}
#endif

TArray<FVector> AMACoverageArea::GetBoundaryPoints() const
{
    TArray<FVector> Points;
    if (!BoundarySpline) return Points;

    int32 NumPoints = BoundarySpline->GetNumberOfSplinePoints();
    for (int32 i = 0; i < NumPoints; ++i)
    {
        Points.Add(BoundarySpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
    }
    return Points;
}

FBox AMACoverageArea::GetAreaBounds() const
{
    TArray<FVector> Points = GetBoundaryPoints();
    if (Points.Num() == 0)
    {
        return FBox(GetActorLocation(), GetActorLocation());
    }

    FBox Bounds(Points[0], Points[0]);
    for (const FVector& Point : Points)
    {
        Bounds += Point;
    }
    return Bounds;
}

bool AMACoverageArea::IsPointInArea(const FVector& Point) const
{
    // 简单的 2D 点在多边形内检测（射线法）
    TArray<FVector> Boundary = GetBoundaryPoints();
    if (Boundary.Num() < 3) return false;

    int32 NumPoints = Boundary.Num();
    bool bInside = false;

    for (int32 i = 0, j = NumPoints - 1; i < NumPoints; j = i++)
    {
        float Xi = Boundary[i].X, Yi = Boundary[i].Y;
        float Xj = Boundary[j].X, Yj = Boundary[j].Y;

        if (((Yi > Point.Y) != (Yj > Point.Y)) &&
            (Point.X < (Xj - Xi) * (Point.Y - Yi) / (Yj - Yi) + Xi))
        {
            bInside = !bInside;
        }
    }

    return bInside;
}


TArray<FVector> AMACoverageArea::GenerateCoveragePath(float InScanRadius) const
{
    // 简单的割草机路径生成（不含NavMesh检查，由Task负责）
    TArray<FVector> Path;
    
    float ActualScanRadius = (InScanRadius > 0.f) ? InScanRadius : DefaultScanRadius;
    
    FBox Bounds = GetAreaBounds();
    if (!Bounds.IsValid) return Path;

    float PathSpacing = ActualScanRadius * 2.f * OverlapFactor;
    if (PathSpacing <= 0.f) PathSpacing = 100.f;

    float MinX = Bounds.Min.X;
    float MaxX = Bounds.Max.X;
    float MinY = Bounds.Min.Y;
    float MaxY = Bounds.Max.Y;
    float Z = (Bounds.Min.Z + Bounds.Max.Z) * 0.5f;

    bool bGoingRight = true;
    float CurrentY = MinY + PathSpacing * 0.5f;

    while (CurrentY <= MaxY)
    {
        if (bGoingRight)
        {
            for (float X = MinX; X <= MaxX; X += PathSpacing)
            {
                FVector Point(X, CurrentY, Z);
                if (IsPointInArea(Point))
                {
                    Path.Add(Point);
                }
            }
        }
        else
        {
            for (float X = MaxX; X >= MinX; X -= PathSpacing)
            {
                FVector Point(X, CurrentY, Z);
                if (IsPointInArea(Point))
                {
                    Path.Add(Point);
                }
            }
        }
        bGoingRight = !bGoingRight;
        CurrentY += PathSpacing;
    }

    return Path;
}

int32 AMACoverageArea::GetCoveragePathCount() const
{
    if (bPathCacheDirty)
    {
        GenerateCoveragePath();
    }
    return CachedCoveragePath.Num();
}

void AMACoverageArea::SetActivePath(const TArray<FVector>& InPath)
{
    ActivePath = InPath;
}

void AMACoverageArea::ClearActivePath()
{
    ActivePath.Empty();
}

void AMACoverageArea::DrawAreaVisualization()
{
    UWorld* World = GetWorld();
    if (!World || !BoundarySpline) return;

    // 绘制边界
    TArray<FVector> Boundary = GetBoundaryPoints();
    int32 NumBoundary = Boundary.Num();
    for (int32 i = 0; i < NumBoundary; ++i)
    {
        FVector Start = Boundary[i];
        FVector End = Boundary[(i + 1) % NumBoundary];
        DrawDebugLine(World, Start, End, BoundaryColor, false, -1.f, 0, LineThickness * 2.f);
    }

    // 绘制实际执行路径（如果有）
    if (ActivePath.Num() > 0)
    {
        for (int32 i = 0; i < ActivePath.Num(); ++i)
        {
            // 绘制路径点
            DrawDebugPoint(World, ActivePath[i], 8.f, PathColor, false, -1.f, 0);

            // 绘制连线
            if (i > 0)
            {
                DrawDebugLine(World, ActivePath[i - 1], ActivePath[i], PathColor, false, -1.f, 0, LineThickness);
            }
        }

        // 绘制起点和终点
        DrawDebugSphere(World, ActivePath[0], 20.f, 8, FColor::Green, false, -1.f, 0, 2.f);
        if (ActivePath.Num() > 1)
        {
            DrawDebugSphere(World, ActivePath.Last(), 20.f, 8, FColor::Red, false, -1.f, 0, 2.f);
        }
    }
}
