// MAPathPlanner.ElevationMap.cpp
// Elevation map grid construction and traversability queries

#include "MAPathPlanner.h"
#include "MAPathPlannerInternal.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"

void FMAElevationMap::Build(
    UWorld* World,
    const FVector& Center,
    float Radius,
    float InCellSize,
    AActor* IgnoreActor,
    float AgentRadius)
{
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("FMAElevationMap::Build - World is null"));
        return;
    }
    
    // 保存栅格参数
    CellSize = FMath::Max(InCellSize, 50.f);
    
    // 计算栅格尺寸
    Width = FMath::CeilToInt((Radius * 2.f) / CellSize);
    Height = Width;  // 正方形栅格
    
    // 限制栅格尺寸
    Width = FMath::Clamp(Width, 10, 200);
    Height = FMath::Clamp(Height, 10, 200);
    
    // 计算栅格原点（左下角）
    Origin.X = Center.X - (Width * CellSize * 0.5f);
    Origin.Y = Center.Y - (Height * CellSize * 0.5f);
    Origin.Z = Center.Z;
    
    // 初始化栅格数组
    Cells.Empty();
    Cells.SetNum(Width);
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(IgnoreActor);
    QueryParams.bTraceComplex = false;
    
    int32 TraversableCount = 0;
    int32 ObstacleCount = 0;
    int32 NoGroundCount = 0;
    
    for (int32 X = 0; X < Width; ++X)
    {
        Cells[X].SetNum(Height);
        
        for (int32 Y = 0; Y < Height; ++Y)
        {
            FMAElevationCell& Cell = Cells[X][Y];
            Cell.ResetSearchState();
            
            // 计算单元格中心的世界坐标
            FVector CellCenter;
            CellCenter.X = Origin.X + (X + 0.5f) * CellSize;
            CellCenter.Y = Origin.Y + (Y + 0.5f) * CellSize;
            CellCenter.Z = Center.Z;
            
            // 从高处向下发射垂直射线检测地面
            FVector RayStart = CellCenter + FVector(0.f, 0.f, MAPathPlannerInternal::ElevationRayStartOffset);
            FVector RayEnd = CellCenter - FVector(0.f, 0.f, MAPathPlannerInternal::ElevationRayEndOffset);
            
            FHitResult HitResult;
            bool bFoundGround = World->LineTraceSingleByChannel(
                HitResult,
                RayStart,
                RayEnd,
                ECC_WorldStatic,
                QueryParams
            );
            
            if (bFoundGround)
            {
                // 记录地面高度
                Cell.GroundZ = HitResult.ImpactPoint.Z;
                
                // 使用球形重叠检测障碍物
                FVector CheckCenter = FVector(CellCenter.X, CellCenter.Y, Cell.GroundZ + MAPathPlannerInternal::ObstacleCheckHeight);
                FCollisionShape SphereShape = FCollisionShape::MakeSphere(AgentRadius);
                
                TArray<FOverlapResult> Overlaps;
                bool bHasObstacle = World->OverlapMultiByChannel(
                    Overlaps,
                    CheckCenter,
                    FQuat::Identity,
                    ECC_WorldDynamic,
                    SphereShape,
                    QueryParams
                );
                
                Cell.bHasObstacle = bHasObstacle && Overlaps.Num() > 0;
                
                // 初步设置可通行性（有地面且无障碍物）
                // 详细的坡度评估在 EvaluateTraversability 中进行
                Cell.bTraversable = !Cell.bHasObstacle;
                
                if (Cell.bHasObstacle)
                {
                    ObstacleCount++;
                }
                else
                {
                    TraversableCount++;
                }
            }
            else
            {
                // 无地面（悬崖、空洞）
                Cell.GroundZ = FLT_MAX;
                Cell.bTraversable = false;
                Cell.bHasObstacle = false;
                NoGroundCount++;
            }
        }
    }
    
    int32 TotalCells = Width * Height;
    UE_LOG(LogTemp, Log, TEXT("FMAElevationMap: Built %dx%d grid (%d cells), traversable: %d, obstacles: %d, no ground: %d"),
           Width, Height, TotalCells, TraversableCount, ObstacleCount, NoGroundCount);
}

bool FMAElevationMap::EvaluateTraversability(
    int32 FromX, int32 FromY,
    int32 ToX, int32 ToY,
    float MaxSlopeAngle,
    float MaxStepHeight) const
{
    // 检查坐标有效性
    if (!IsValidCell(FromX, FromY) || !IsValidCell(ToX, ToY))
    {
        return false;
    }
    
    const FMAElevationCell& FromCell = Cells[FromX][FromY];
    const FMAElevationCell& ToCell = Cells[ToX][ToY];
    
    // 规则 1: 目标格必须有地面
    if (!ToCell.HasGround())
    {
        return false;
    }
    
    // 规则 2: 目标格不能有障碍物
    if (ToCell.bHasObstacle)
    {
        return false;
    }
    
    // 规则 3: 起始格必须有地面
    if (!FromCell.HasGround())
    {
        return false;
    }
    
    // 规则 4: 高度差检查（台阶检测）
    float HeightDiff = FMath::Abs(ToCell.GroundZ - FromCell.GroundZ);
    if (HeightDiff > MaxStepHeight)
    {
        return false;
    }
    
    // 规则 5: 坡度检查
    float HorizontalDist = CellSize;
    // 对角线移动时水平距离更长
    if (FromX != ToX && FromY != ToY)
    {
        HorizontalDist = CellSize * 1.414f;  // sqrt(2)
    }
    
    float SlopeAngle = FMath::RadiansToDegrees(FMath::Atan2(HeightDiff, HorizontalDist));
    if (SlopeAngle > MaxSlopeAngle)
    {
        return false;
    }
    
    return true;
}

float FMAElevationMap::GetGroundHeight(int32 X, int32 Y) const
{
    if (!IsValidCell(X, Y))
    {
        return FLT_MAX;
    }
    return Cells[X][Y].GroundZ;
}

bool FMAElevationMap::IsTraversable(int32 X, int32 Y) const
{
    if (!IsValidCell(X, Y))
    {
        return false;
    }
    return Cells[X][Y].bTraversable;
}

bool FMAElevationMap::HasGround(int32 X, int32 Y) const
{
    if (!IsValidCell(X, Y))
    {
        return false;
    }
    return Cells[X][Y].HasGround();
}

bool FMAElevationMap::IsValidCell(int32 X, int32 Y) const
{
    return X >= 0 && X < Width && Y >= 0 && Y < Height;
}

FIntPoint FMAElevationMap::WorldToGrid(const FVector& WorldPos) const
{
    float RelativeX = WorldPos.X - Origin.X;
    float RelativeY = WorldPos.Y - Origin.Y;
    
    int32 GridX = FMath::FloorToInt(RelativeX / CellSize);
    int32 GridY = FMath::FloorToInt(RelativeY / CellSize);
    
    return FIntPoint(GridX, GridY);
}

FVector FMAElevationMap::GridToWorld(int32 X, int32 Y) const
{
    FVector WorldPos;
    WorldPos.X = Origin.X + (X + 0.5f) * CellSize;
    WorldPos.Y = Origin.Y + (Y + 0.5f) * CellSize;
    
    // 如果有有效地面高度，使用它；否则使用原点高度
    if (IsValidCell(X, Y) && Cells[X][Y].HasGround())
    {
        WorldPos.Z = Cells[X][Y].GroundZ;
    }
    else
    {
        WorldPos.Z = Origin.Z;
    }
    
    return WorldPos;
}

TArray<FIntPoint> FMAElevationMap::GetNeighbors(int32 X, int32 Y) const
{
    TArray<FIntPoint> Neighbors;
    Neighbors.Reserve(8);
    
    // 8 方向邻居
    static const int32 DX[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const int32 DY[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    for (int32 i = 0; i < 8; ++i)
    {
        int32 NX = X + DX[i];
        int32 NY = Y + DY[i];
        
        if (IsValidCell(NX, NY))
        {
            Neighbors.Add(FIntPoint(NX, NY));
        }
    }
    
    return Neighbors;
}

void FMAElevationMap::ResetSearchState()
{
    for (int32 X = 0; X < Width; ++X)
    {
        for (int32 Y = 0; Y < Height; ++Y)
        {
            Cells[X][Y].ResetSearchState();
        }
    }
}

FMAElevationCell& FMAElevationMap::GetCell(int32 X, int32 Y)
{
    check(IsValidCell(X, Y));
    return Cells[X][Y];
}

const FMAElevationCell& FMAElevationMap::GetCell(int32 X, int32 Y) const
{
    check(IsValidCell(X, Y));
    return Cells[X][Y];
}

