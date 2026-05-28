// MAPatrolPath.cpp

#include "MAPatrolPath.h"
#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"
#include "DrawDebugHelpers.h"
#include "../../UI/Core/MAUIManager.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

AMAPatrolPath::AMAPatrolPath()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

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
    
    // 运行时不需要每帧 Tick，只在需要可视化时启用
    SetActorTickEnabled(bShowPathAtRuntime);
}

void AMAPatrolPath::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (bShowPathAtRuntime)
    {
        DrawPathVisualization();
    }
}

void AMAPatrolPath::DrawPathVisualization()
{
    UWorld* World = GetWorld();
    if (!World || !PathSpline) return;
    
    int32 NumPoints = PathSpline->GetNumberOfSplinePoints();
    if (NumPoints < 2) return;
    
    // 检查是否有遮挡性 Widget 可见，如果有则不绘制调试文字（避免透出）
    bool bShouldDrawText = bShowWaypointNumbers;
    if (bShouldDrawText)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
        if (PC)
        {
            // 检查是否有可见的 Widget（排除 HUDWidget 等始终可见的 Widget）
            TArray<UUserWidget*> FoundWidgets;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, FoundWidgets, UUserWidget::StaticClass(), false);
            for (UUserWidget* Widget : FoundWidgets)
            {
                if (Widget && Widget->IsVisible() && Widget->GetVisibility() == ESlateVisibility::Visible)
                {
                    // 排除 HUDWidget（它使用 SelfHitTestInvisible，不会遮挡）
                    // 检查 Widget 名称或类型
                    FString WidgetName = Widget->GetClass()->GetName();
                    if (!WidgetName.Contains(TEXT("HUDWidget")))
                    {
                        // 有可见的遮挡性 Widget，不绘制调试文字
                        bShouldDrawText = false;
                        break;
                    }
                }
            }
        }
    }
    
    // 绘制路径点和连线
    for (int32 i = 0; i < NumPoints; ++i)
    {
        FVector CurrentPoint = PathSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        
        // 绘制路径点（球体）
        DrawDebugSphere(World, CurrentPoint, WaypointSize, 12, WaypointColor, false, -1.f, 0, 2.f);
        
        // 绘制路径点编号
        // 修复闪烁问题：bPersistent = false，Duration = 0.05f
        if (bShouldDrawText)
        {
            FString PointLabel = FString::Printf(TEXT("%d"), i + 1);
            DrawDebugString(World, CurrentPoint + FVector(0, 0, WaypointSize + 20.f), PointLabel, nullptr, WaypointColor, 0.05f, false, 1.2f);
        }
        
        // 绘制到下一个点的连线
        int32 NextIndex = (i + 1) % NumPoints;
        if (NextIndex != 0 || bClosedLoop)
        {
            FVector NextPoint = PathSpline->GetLocationAtSplinePoint(NextIndex, ESplineCoordinateSpace::World);
            DrawDebugLine(World, CurrentPoint, NextPoint, PathColor, false, -1.f, 0, PathThickness);
            
            // 绘制方向箭头（在线段中点）
            FVector MidPoint = (CurrentPoint + NextPoint) * 0.5f;
            FVector Direction = (NextPoint - CurrentPoint).GetSafeNormal();
            DrawDebugDirectionalArrow(World, MidPoint - Direction * 30.f, MidPoint + Direction * 30.f, 40.f, PathColor, false, -1.f, 0, PathThickness);
        }
    }
    
    // 绘制起点标记（稍大一点的圆圈）
    FVector StartPoint = PathSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
    DrawDebugCircle(World, StartPoint, WaypointSize * 1.5f, 24, FColor::Green, false, -1.f, 0, PathThickness, FVector(1, 0, 0), FVector(0, 1, 0), false);
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
