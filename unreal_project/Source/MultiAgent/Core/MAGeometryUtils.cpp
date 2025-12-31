// MAGeometryUtils.cpp
// 几何计算工具类实现
// Requirements: 6.1, 6.2, 6.3, 7.1, 7.2, 7.3, 3.2, 4.2, 5.2, 5.3, 5.4

#include "MAGeometryUtils.h"
#include "GameFramework/Actor.h"
#include "OrientedBoxTypes.h"
#include "BoxTypes.h"
#include "Algo/Reverse.h"

//=============================================================================
// 辅助方法
//=============================================================================

float FMAGeometryUtils::CrossProduct2D(const FVector2D& O, const FVector2D& A, const FVector2D& B)
{
    // 计算向量 OA 和 OB 的叉积: (A-O) × (B-O)
    // = (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X)
    return (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X);
}

bool FMAGeometryUtils::CompareByPolarAngle(const FVector2D& Pivot, const FVector2D& A, const FVector2D& B)
{
    float Cross = CrossProduct2D(Pivot, A, B);
    
    if (FMath::IsNearlyZero(Cross))
    {
        // 共线时，距离近的排在前面
        float DistA = FVector2D::DistSquared(Pivot, A);
        float DistB = FVector2D::DistSquared(Pivot, B);
        return DistA < DistB;
    }
    
    // 逆时针方向 (叉积 > 0) 排在前面
    return Cross > 0;
}

//=============================================================================
// 边界框角点收集 (Requirements: 6.1)
//=============================================================================

TArray<FVector2D> FMAGeometryUtils::CollectBoundingBoxCorners(const TArray<AActor*>& Actors)
{
    TArray<FVector2D> AllCorners;
    
    for (AActor* Actor : Actors)
    {
        if (!Actor)
        {
            continue;
        }
        
        // 获取 Actor 的边界框
        FVector Origin;
        FVector BoxExtent;
        Actor->GetActorBounds(false, Origin, BoxExtent);
        
        // 计算 8 个角点的 3D 坐标
        // BoxExtent 是半边长，Origin 是中心点
        TArray<FVector> Corners3D;
        Corners3D.Reserve(8);
        
        for (int32 i = 0; i < 8; ++i)
        {
            FVector Corner = Origin;
            Corner.X += (i & 1) ? BoxExtent.X : -BoxExtent.X;
            Corner.Y += (i & 2) ? BoxExtent.Y : -BoxExtent.Y;
            Corner.Z += (i & 4) ? BoxExtent.Z : -BoxExtent.Z;
            Corners3D.Add(Corner);
        }
        
        // 投影到 XY 平面
        for (const FVector& Corner3D : Corners3D)
        {
            AllCorners.Add(FVector2D(Corner3D.X, Corner3D.Y));
        }
    }
    
    return AllCorners;
}

//=============================================================================
// 凸包计算 - Graham Scan 算法 (Requirements: 6.2, 6.3)
//=============================================================================

TArray<FVector2D> FMAGeometryUtils::ComputeConvexHull2D(const TArray<FVector2D>& Points)
{
    TArray<FVector2D> Hull;
    
    // 处理退化情况
    if (Points.Num() < 3)
    {
        // 少于 3 个点，直接返回所有点
        Hull = Points;
        return Hull;
    }
    
    // 复制点集以便排序
    TArray<FVector2D> SortedPoints = Points;
    
    // Step 1: 找到最低点 (Y 最小，相同 Y 时 X 最小) 作为起点
    int32 LowestIndex = 0;
    for (int32 i = 1; i < SortedPoints.Num(); ++i)
    {
        if (SortedPoints[i].Y < SortedPoints[LowestIndex].Y ||
            (FMath::IsNearlyEqual(SortedPoints[i].Y, SortedPoints[LowestIndex].Y) && 
             SortedPoints[i].X < SortedPoints[LowestIndex].X))
        {
            LowestIndex = i;
        }
    }
    
    // 将最低点移到数组开头
    FVector2D Pivot = SortedPoints[LowestIndex];
    SortedPoints.RemoveAt(LowestIndex);
    
    // Step 2: 按极角排序其他点
    SortedPoints.Sort([&Pivot](const FVector2D& A, const FVector2D& B)
    {
        return CompareByPolarAngle(Pivot, A, B);
    });
    
    // 移除共线的点，只保留最远的
    TArray<FVector2D> FilteredPoints;
    FilteredPoints.Add(Pivot);
    
    for (int32 i = 0; i < SortedPoints.Num(); ++i)
    {
        // 跳过与 Pivot 重合的点
        if (FVector2D::DistSquared(Pivot, SortedPoints[i]) < KINDA_SMALL_NUMBER)
        {
            continue;
        }
        
        // 检查是否与前一个点共线
        while (i < SortedPoints.Num() - 1)
        {
            float Cross = CrossProduct2D(Pivot, SortedPoints[i], SortedPoints[i + 1]);
            if (FMath::IsNearlyZero(Cross))
            {
                // 共线，跳过当前点，保留更远的点
                ++i;
            }
            else
            {
                break;
            }
        }
        
        if (i < SortedPoints.Num())
        {
            FilteredPoints.Add(SortedPoints[i]);
        }
    }
    
    // 如果过滤后点数不足，返回所有点
    if (FilteredPoints.Num() < 3)
    {
        return FilteredPoints;
    }
    
    // Step 3: 使用栈构建凸包
    Hull.Add(FilteredPoints[0]);
    Hull.Add(FilteredPoints[1]);
    
    for (int32 i = 2; i < FilteredPoints.Num(); ++i)
    {
        // 移除所有使得转向为顺时针或共线的点
        while (Hull.Num() > 1)
        {
            float Cross = CrossProduct2D(Hull[Hull.Num() - 2], Hull[Hull.Num() - 1], FilteredPoints[i]);
            if (Cross <= 0)
            {
                // 顺时针或共线，弹出栈顶
                Hull.Pop();
            }
            else
            {
                break;
            }
        }
        Hull.Add(FilteredPoints[i]);
    }
    
    // Step 4: 返回逆时针顺序的顶点 (Graham Scan 自然产生逆时针顺序)
    return Hull;
}

//=============================================================================
// 最近邻排序 (Requirements: 7.1, 7.2, 7.3)
//=============================================================================

TArray<FVector2D> FMAGeometryUtils::SortByNearestNeighbor(const TArray<FVector2D>& Points)
{
    TArray<FVector2D> Result;
    
    if (Points.Num() == 0)
    {
        return Result;
    }
    
    if (Points.Num() == 1)
    {
        Result.Add(Points[0]);
        return Result;
    }
    
    // 创建未访问点集合
    TArray<FVector2D> Unvisited = Points;
    
    // Step 1: 找到最小 X 坐标点作为起点 (相同 X 时选择较小 Y)
    int32 StartIndex = 0;
    for (int32 i = 1; i < Unvisited.Num(); ++i)
    {
        if (Unvisited[i].X < Unvisited[StartIndex].X ||
            (FMath::IsNearlyEqual(Unvisited[i].X, Unvisited[StartIndex].X) && 
             Unvisited[i].Y < Unvisited[StartIndex].Y))
        {
            StartIndex = i;
        }
    }
    
    // 将起点加入结果并从未访问集合移除
    FVector2D CurrentPoint = Unvisited[StartIndex];
    Result.Add(CurrentPoint);
    Unvisited.RemoveAt(StartIndex);
    
    // Step 2 & 3: 贪心选择最近未访问点
    while (Unvisited.Num() > 0)
    {
        int32 NearestIndex = 0;
        float NearestDistSq = FVector2D::DistSquared(CurrentPoint, Unvisited[0]);
        
        for (int32 i = 1; i < Unvisited.Num(); ++i)
        {
            float DistSq = FVector2D::DistSquared(CurrentPoint, Unvisited[i]);
            
            if (DistSq < NearestDistSq ||
                (FMath::IsNearlyEqual(DistSq, NearestDistSq) && 
                 Unvisited[i].Y < Unvisited[NearestIndex].Y))
            {
                // 更近，或者等距时选择较小 Y 坐标点
                NearestIndex = i;
                NearestDistSq = DistSq;
            }
        }
        
        // 将最近点加入结果
        CurrentPoint = Unvisited[NearestIndex];
        Result.Add(CurrentPoint);
        Unvisited.RemoveAt(NearestIndex);
    }
    
    return Result;
}

//=============================================================================
// 3D 边界框顶点收集 (Requirements: 3.3, 6.2)
//=============================================================================

TArray<FVector> FMAGeometryUtils::CollectBoundingBoxVertices3D(const TArray<AActor*>& Actors)
{
    TArray<FVector> AllVertices;
    
    for (AActor* Actor : Actors)
    {
        if (!Actor)
        {
            continue;
        }
        
        // 获取 Actor 的边界框
        FVector Origin;
        FVector BoxExtent;
        Actor->GetActorBounds(false, Origin, BoxExtent);
        
        // 计算 8 个角点的 3D 坐标
        // BoxExtent 是半边长，Origin 是中心点
        for (int32 i = 0; i < 8; ++i)
        {
            FVector Corner = Origin;
            Corner.X += (i & 1) ? BoxExtent.X : -BoxExtent.X;
            Corner.Y += (i & 2) ? BoxExtent.Y : -BoxExtent.Y;
            Corner.Z += (i & 4) ? BoxExtent.Z : -BoxExtent.Z;
            AllVertices.Add(Corner);
        }
    }
    
    return AllVertices;
}


//=============================================================================
// Prism 几何计算 (Requirements: 2.3, 2.4, 6.2, 6.3, 6.4)
//=============================================================================

FMAPrismGeometry FMAGeometryUtils::ComputePrismFromActors(const TArray<AActor*>& Actors)
{
    FMAPrismGeometry Result;
    
    if (Actors.Num() == 0)
    {
        return Result;
    }
    
    // Step 1: 收集所有 3D 顶点
    TArray<FVector> Vertices3D = CollectBoundingBoxVertices3D(Actors);
    
    if (Vertices3D.Num() == 0)
    {
        return Result;
    }
    
    // Step 2: 计算高度 (maxZ - minZ)
    float MinZ = Vertices3D[0].Z;
    float MaxZ = Vertices3D[0].Z;
    
    for (const FVector& Vertex : Vertices3D)
    {
        MinZ = FMath::Min(MinZ, Vertex.Z);
        MaxZ = FMath::Max(MaxZ, Vertex.Z);
    }
    
    Result.Height = MaxZ - MinZ;
    
    // Step 3: 将顶点投影到 XY 平面
    TArray<FVector2D> Vertices2D;
    Vertices2D.Reserve(Vertices3D.Num());
    
    for (const FVector& Vertex : Vertices3D)
    {
        Vertices2D.Add(FVector2D(Vertex.X, Vertex.Y));
    }
    
    // Step 4: 计算凸包作为底面多边形
    TArray<FVector2D> ConvexHull = ComputeConvexHull2D(Vertices2D);
    
    if (ConvexHull.Num() < 3)
    {
        // 凸包点数不足，无法形成有效多边形
        return Result;
    }
    
    // Step 5: 转换为 3D 顶点 (Z = MinZ，即底面)
    // ComputeConvexHull2D 已经返回逆时针顺序
    Result.BottomVertices.Reserve(ConvexHull.Num());
    
    for (const FVector2D& Point2D : ConvexHull)
    {
        Result.BottomVertices.Add(FVector(Point2D.X, Point2D.Y, MinZ));
    }
    
    Result.bIsValid = true;
    return Result;
}


//=============================================================================
// OBB 底面角点提取 (Requirements: 5.4)
//=============================================================================

TArray<FVector> FMAGeometryUtils::ExtractBottomCorners(const UE::Geometry::TOrientedBox3<double>& OBB)
{
    TArray<FVector> BottomCorners;
    
    // 获取 OBB 的 8 个角点
    TArray<FVector> AllCorners;
    AllCorners.Reserve(8);
    
    // OBB 的中心和半边长
    UE::Math::TVector<double> Center = OBB.Frame.Origin;
    UE::Math::TVector<double> Extents = OBB.Extents;
    
    // 获取 OBB 的三个轴向量
    UE::Math::TVector<double> AxisX = OBB.Frame.GetAxis(0);
    UE::Math::TVector<double> AxisY = OBB.Frame.GetAxis(1);
    UE::Math::TVector<double> AxisZ = OBB.Frame.GetAxis(2);
    
    // 计算 8 个角点
    for (int32 i = 0; i < 8; ++i)
    {
        double SignX = (i & 1) ? 1.0 : -1.0;
        double SignY = (i & 2) ? 1.0 : -1.0;
        double SignZ = (i & 4) ? 1.0 : -1.0;
        
        UE::Math::TVector<double> Corner = Center 
            + AxisX * Extents.X * SignX
            + AxisY * Extents.Y * SignY
            + AxisZ * Extents.Z * SignZ;
        
        AllCorners.Add(FVector(Corner.X, Corner.Y, Corner.Z));
    }
    
    // 找到最小 Z 值
    float MinZ = AllCorners[0].Z;
    for (const FVector& Corner : AllCorners)
    {
        MinZ = FMath::Min(MinZ, Corner.Z);
    }
    
    // 筛选 Z 坐标接近最小值的 4 个点作为底面角点
    // 使用容差来处理浮点精度问题
    const float ZTolerance = 1.0f;
    
    for (const FVector& Corner : AllCorners)
    {
        if (FMath::Abs(Corner.Z - MinZ) < ZTolerance)
        {
            BottomCorners.Add(Corner);
        }
    }
    
    // 如果没有找到恰好 4 个点，选择 Z 最小的 4 个点
    if (BottomCorners.Num() != 4)
    {
        BottomCorners.Empty();
        
        // 按 Z 坐标排序
        AllCorners.Sort([](const FVector& A, const FVector& B)
        {
            return A.Z < B.Z;
        });
        
        // 取前 4 个
        for (int32 i = 0; i < FMath::Min(4, AllCorners.Num()); ++i)
        {
            BottomCorners.Add(AllCorners[i]);
        }
    }
    
    // 按逆时针顺序排列底面角点
    if (BottomCorners.Num() >= 3)
    {
        // 计算中心点
        FVector CenterPoint = FVector::ZeroVector;
        for (const FVector& Corner : BottomCorners)
        {
            CenterPoint += Corner;
        }
        CenterPoint /= BottomCorners.Num();
        
        // 按极角排序 (逆时针)
        BottomCorners.Sort([&CenterPoint](const FVector& A, const FVector& B)
        {
            float AngleA = FMath::Atan2(A.Y - CenterPoint.Y, A.X - CenterPoint.X);
            float AngleB = FMath::Atan2(B.Y - CenterPoint.Y, B.X - CenterPoint.X);
            return AngleA < AngleB;
        });
    }
    
    return BottomCorners;
}


//=============================================================================
// OBB 几何计算 (Requirements: 3.2, 5.2, 5.3)
//=============================================================================

FMAOBBGeometry FMAGeometryUtils::ComputeOBBFromActors(const TArray<AActor*>& Actors)
{
    FMAOBBGeometry Result;
    
    if (Actors.Num() == 0)
    {
        return Result;
    }
    
    // Step 1: 收集所有 3D 顶点
    TArray<FVector> Vertices3D = CollectBoundingBoxVertices3D(Actors);
    
    if (Vertices3D.Num() < 3)
    {
        return Result;
    }
    
    // Step 2: 转换为 GeometryCore 的点格式
    TArray<UE::Math::TVector<double>> Points;
    Points.Reserve(Vertices3D.Num());
    
    for (const FVector& Vertex : Vertices3D)
    {
        Points.Add(UE::Math::TVector<double>(Vertex.X, Vertex.Y, Vertex.Z));
    }
    
    // Step 3: 使用 FOrientedBox3d 拟合 OBB
    UE::Geometry::TOrientedBox3<double> OBB;
    
    // 计算点集的 AABB 作为初始估计
    UE::Math::TVector<double> MinPoint = Points[0];
    UE::Math::TVector<double> MaxPoint = Points[0];
    
    for (const auto& Point : Points)
    {
        MinPoint.X = FMath::Min(MinPoint.X, Point.X);
        MinPoint.Y = FMath::Min(MinPoint.Y, Point.Y);
        MinPoint.Z = FMath::Min(MinPoint.Z, Point.Z);
        MaxPoint.X = FMath::Max(MaxPoint.X, Point.X);
        MaxPoint.Y = FMath::Max(MaxPoint.Y, Point.Y);
        MaxPoint.Z = FMath::Max(MaxPoint.Z, Point.Z);
    }
    
    // 设置 OBB 参数
    UE::Math::TVector<double> Center = (MinPoint + MaxPoint) * 0.5;
    UE::Math::TVector<double> Extents = (MaxPoint - MinPoint) * 0.5;
    
    // 使用单位轴 (AABB 近似)
    OBB.Frame = UE::Geometry::TFrame3<double>(Center);
    OBB.Extents = Extents;
    
    // Step 4: 提取底面四个角点
    Result.CornerPoints = ExtractBottomCorners(OBB);
    
    // 设置其他属性
    Result.Center = FVector(Center.X, Center.Y, Center.Z);
    Result.Extent = FVector(Extents.X, Extents.Y, Extents.Z);
    Result.Orientation = FQuat::Identity; // AABB 近似使用单位四元数
    Result.bIsValid = (Result.CornerPoints.Num() == 4);
    
    return Result;
}


//=============================================================================
// 几何中心计算 (Requirements: 4.2)
//=============================================================================

FVector FMAGeometryUtils::ComputeCenterFromActors(const TArray<AActor*>& Actors)
{
    FVector Center = FVector::ZeroVector;
    
    if (Actors.Num() == 0)
    {
        return Center;
    }
    
    int32 ValidCount = 0;
    
    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            Center += Actor->GetActorLocation();
            ++ValidCount;
        }
    }
    
    if (ValidCount > 0)
    {
        Center /= ValidCount;
    }
    
    return Center;
}

//=============================================================================
// 矩形短边中点计算 (用于 trans_facility linestring)
//=============================================================================

TArray<FVector> FMAGeometryUtils::ComputeShortEdgeMidpoints(const TArray<FVector>& CornerPoints)
{
    TArray<FVector> Midpoints;
    
    // 需要恰好 4 个角点
    if (CornerPoints.Num() != 4)
    {
        return Midpoints;
    }
    
    // 角点按逆时针顺序排列: P0 -> P1 -> P2 -> P3
    // 边: E0 = P0-P1, E1 = P1-P2, E2 = P2-P3, E3 = P3-P0
    // 对边: (E0, E2) 和 (E1, E3)
    
    // 计算相邻边的长度
    float Edge01Length = FVector::Dist(CornerPoints[0], CornerPoints[1]);
    float Edge12Length = FVector::Dist(CornerPoints[1], CornerPoints[2]);
    
    // 判断哪对边是短边
    // 如果 Edge01 < Edge12，则 (E0, E2) 是短边，中点是 E0 和 E2 的中点
    // 否则 (E1, E3) 是短边，中点是 E1 和 E3 的中点
    
    if (Edge01Length < Edge12Length)
    {
        // 短边是 E0 (P0-P1) 和 E2 (P2-P3)
        FVector Midpoint0 = (CornerPoints[0] + CornerPoints[1]) * 0.5f;
        FVector Midpoint1 = (CornerPoints[2] + CornerPoints[3]) * 0.5f;
        Midpoints.Add(Midpoint0);
        Midpoints.Add(Midpoint1);
    }
    else
    {
        // 短边是 E1 (P1-P2) 和 E3 (P3-P0)
        FVector Midpoint0 = (CornerPoints[1] + CornerPoints[2]) * 0.5f;
        FVector Midpoint1 = (CornerPoints[3] + CornerPoints[0]) * 0.5f;
        Midpoints.Add(Midpoint0);
        Midpoints.Add(Midpoint1);
    }
    
    return Midpoints;
}
