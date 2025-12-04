// MAPatrolPath.cpp

#include "MAPatrolPath.h"
#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"

AMAPatrolPath::AMAPatrolPath()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建根组件 Billboard（编辑器中显示图标）
    Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    RootComponent = Billboard;

    // 创建 Spline 组件
    PathSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PathSpline"));
    PathSpline->SetupAttachment(RootComponent);
    
    // 设置 Spline 可视化
    PathSpline->SetDrawDebug(true);
    PathSpline->SetUnselectedSplineSegmentColor(FLinearColor(0.f, 1.f, 1.f));  // 青色
    PathSpline->SetSelectedSplineSegmentColor(FLinearColor(1.f, 1.f, 0.f));    // 黄色
    PathSpline->SetTangentColor(FLinearColor(1.f, 0.5f, 0.f));                 // 橙色
    
    // 设置为线性插值（直线路径）
    PathSpline->SetSplinePointType(0, ESplinePointType::Linear);
    
    // 清除默认点，添加初始路径点
    PathSpline->ClearSplinePoints(false);
    PathSpline->AddSplinePoint(FVector(0.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
    PathSpline->AddSplinePoint(FVector(500.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
    PathSpline->AddSplinePoint(FVector(500.f, 500.f, 0.f), ESplineCoordinateSpace::Local, false);
    PathSpline->AddSplinePoint(FVector(0.f, 500.f, 0.f), ESplineCoordinateSpace::Local, false);
    
    // 设置闭合循环
    PathSpline->SetClosedLoop(bClosedLoop);
    PathSpline->UpdateSpline();
}

void AMAPatrolPath::BeginPlay()
{
    Super::BeginPlay();
    UpdateWaypointsFromSpline();
}

void AMAPatrolPath::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    if (PathSpline)
    {
        PathSpline->SetClosedLoop(bClosedLoop);
        
        // 设置所有点为线性
        for (int32 i = 0; i < PathSpline->GetNumberOfSplinePoints(); ++i)
        {
            PathSpline->SetSplinePointType(i, ESplinePointType::Linear, false);
        }
        PathSpline->UpdateSpline();
    }
    
    UpdateWaypointsFromSpline();
}

#if WITH_EDITOR
void AMAPatrolPath::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (PathSpline)
    {
        PathSpline->SetClosedLoop(bClosedLoop);
        PathSpline->UpdateSpline();
    }
    
    UpdateWaypointsFromSpline();
}
#endif

void AMAPatrolPath::UpdateWaypointsFromSpline()
{
    Waypoints.Empty();
    
    if (!PathSpline) return;
    
    int32 NumPoints = PathSpline->GetNumberOfSplinePoints();
    for (int32 i = 0; i < NumPoints; ++i)
    {
        FVector WorldPos = PathSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        Waypoints.Add(WorldPos);
    }
}

TArray<FVector> AMAPatrolPath::GetWaypoints() const
{
    TArray<FVector> Result;
    
    if (!PathSpline) return Result;
    
    int32 NumPoints = PathSpline->GetNumberOfSplinePoints();
    for (int32 i = 0; i < NumPoints; ++i)
    {
        FVector WorldPos = PathSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        Result.Add(WorldPos);
    }
    
    return Result;
}

FVector AMAPatrolPath::GetWaypoint(int32 Index) const
{
    if (!PathSpline || PathSpline->GetNumberOfSplinePoints() == 0)
    {
        return GetActorLocation();
    }
    
    Index = FMath::Clamp(Index, 0, PathSpline->GetNumberOfSplinePoints() - 1);
    return PathSpline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
}

int32 AMAPatrolPath::GetNextWaypointIndex(int32 CurrentIndex) const
{
    int32 NumPoints = GetWaypointCount();
    if (NumPoints == 0) return 0;
    
    if (bClosedLoop)
    {
        return (CurrentIndex + 1) % NumPoints;
    }
    else
    {
        return FMath::Min(CurrentIndex + 1, NumPoints - 1);
    }
}

int32 AMAPatrolPath::GetWaypointCount() const
{
    return PathSpline ? PathSpline->GetNumberOfSplinePoints() : 0;
}
