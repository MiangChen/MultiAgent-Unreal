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
    return (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X);
}

bool FMAGeometryUtils::CompareByPolarAngle(const FVector2D& Pivot, const FVector2D& A, const FVector2D& B)
{
    float Cross = CrossProduct2D(Pivot, A, B);
    
    if (FMath::IsNearlyZero(Cross))
    {
        float DistA = FVector2D::DistSquared(Pivot, A);
        float DistB = FVector2D::DistSquared(Pivot, B);
        return DistA < DistB;
    }
    
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
        if (!Actor) continue;
        
        FVector Origin;
        FVector BoxExtent;
        Actor->GetActorBounds(false, Origin, BoxExtent);

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
    
    if (Points.Num() < 3)
    {
        Hull = Points;
        return Hull;
    }
    
    TArray<FVector2D> SortedPoints = Points;
    
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
    
    FVector2D Pivot = SortedPoints[LowestIndex];
    SortedPoints.RemoveAt(LowestIndex);
    
    SortedPoints.Sort([&Pivot](const FVector2D& A, const FVector2D& B)
    {
        return CompareByPolarAngle(Pivot, A, B);
    });

    TArray<FVector2D> FilteredPoints;
    FilteredPoints.Add(Pivot);
    
    for (int32 i = 0; i < SortedPoints.Num(); ++i)
    {
        if (FVector2D::DistSquared(Pivot, SortedPoints[i]) < KINDA_SMALL_NUMBER)
        {
            continue;
        }
        
        while (i < SortedPoints.Num() - 1)
        {
            float Cross = CrossProduct2D(Pivot, SortedPoints[i], SortedPoints[i + 1]);
            if (FMath::IsNearlyZero(Cross))
            {
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
    
    if (FilteredPoints.Num() < 3)
    {
        return FilteredPoints;
    }
    
    Hull.Add(FilteredPoints[0]);
    Hull.Add(FilteredPoints[1]);
    
    for (int32 i = 2; i < FilteredPoints.Num(); ++i)
    {
        while (Hull.Num() > 1)
        {
            float Cross = CrossProduct2D(Hull[Hull.Num() - 2], Hull[Hull.Num() - 1], FilteredPoints[i]);
            if (Cross <= 0)
            {
                Hull.Pop();
            }
            else
            {
                break;
            }
        }
        Hull.Add(FilteredPoints[i]);
    }
    
    return Hull;
}


//=============================================================================
// 最近邻排序 (Requirements: 7.1, 7.2, 7.3)
//=============================================================================

TArray<FVector2D> FMAGeometryUtils::SortByNearestNeighbor(const TArray<FVector2D>& Points)
{
    TArray<FVector2D> Result;
    
    if (Points.Num() == 0) return Result;
    if (Points.Num() == 1) { Result.Add(Points[0]); return Result; }
    
    TArray<FVector2D> Unvisited = Points;
    
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
    
    FVector2D CurrentPoint = Unvisited[StartIndex];
    Result.Add(CurrentPoint);
    Unvisited.RemoveAt(StartIndex);
    
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
                NearestIndex = i;
                NearestDistSq = DistSq;
            }
        }
        
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
        if (!Actor) continue;
        
        FVector Origin;
        FVector BoxExtent;
        Actor->GetActorBounds(false, Origin, BoxExtent);
        
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
    
    if (Actors.Num() == 0) return Result;
    
    TArray<FVector> Vertices3D = CollectBoundingBoxVertices3D(Actors);
    if (Vertices3D.Num() == 0) return Result;
    
    float MinZ = Vertices3D[0].Z;
    float MaxZ = Vertices3D[0].Z;
    
    for (const FVector& Vertex : Vertices3D)
    {
        MinZ = FMath::Min(MinZ, Vertex.Z);
        MaxZ = FMath::Max(MaxZ, Vertex.Z);
    }
    
    Result.Height = MaxZ - MinZ;

    TArray<FVector2D> Vertices2D;
    Vertices2D.Reserve(Vertices3D.Num());
    
    for (const FVector& Vertex : Vertices3D)
    {
        Vertices2D.Add(FVector2D(Vertex.X, Vertex.Y));
    }
    
    TArray<FVector2D> ConvexHull = ComputeConvexHull2D(Vertices2D);
    
    if (ConvexHull.Num() < 3) return Result;
    
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
    TArray<FVector> AllCorners;
    AllCorners.Reserve(8);
    
    UE::Math::TVector<double> Center = OBB.Frame.Origin;
    UE::Math::TVector<double> Extents = OBB.Extents;
    
    UE::Math::TVector<double> AxisX = OBB.Frame.GetAxis(0);
    UE::Math::TVector<double> AxisY = OBB.Frame.GetAxis(1);
    UE::Math::TVector<double> AxisZ = OBB.Frame.GetAxis(2);
    
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

    float MinZ = AllCorners[0].Z;
    for (const FVector& Corner : AllCorners)
    {
        MinZ = FMath::Min(MinZ, Corner.Z);
    }
    
    const float ZTolerance = 1.0f;
    
    for (const FVector& Corner : AllCorners)
    {
        if (FMath::Abs(Corner.Z - MinZ) < ZTolerance)
        {
            BottomCorners.Add(Corner);
        }
    }
    
    if (BottomCorners.Num() != 4)
    {
        BottomCorners.Empty();
        
        AllCorners.Sort([](const FVector& A, const FVector& B)
        {
            return A.Z < B.Z;
        });
        
        for (int32 i = 0; i < FMath::Min(4, AllCorners.Num()); ++i)
        {
            BottomCorners.Add(AllCorners[i]);
        }
    }
    
    if (BottomCorners.Num() >= 3)
    {
        FVector CenterPoint = FVector::ZeroVector;
        for (const FVector& Corner : BottomCorners)
        {
            CenterPoint += Corner;
        }
        CenterPoint /= BottomCorners.Num();
        
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
    
    if (Actors.Num() == 0) return Result;
    
    TArray<FVector> Vertices3D = CollectBoundingBoxVertices3D(Actors);
    if (Vertices3D.Num() < 3) return Result;
    
    TArray<UE::Math::TVector<double>> Points;
    Points.Reserve(Vertices3D.Num());
    
    for (const FVector& Vertex : Vertices3D)
    {
        Points.Add(UE::Math::TVector<double>(Vertex.X, Vertex.Y, Vertex.Z));
    }
    
    UE::Geometry::TOrientedBox3<double> OBB;
    
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
    
    UE::Math::TVector<double> Center = (MinPoint + MaxPoint) * 0.5;
    UE::Math::TVector<double> Extents = (MaxPoint - MinPoint) * 0.5;
    
    OBB.Frame = UE::Geometry::TFrame3<double>(Center);
    OBB.Extents = Extents;
    
    Result.CornerPoints = ExtractBottomCorners(OBB);
    Result.Center = FVector(Center.X, Center.Y, Center.Z);
    Result.Extent = FVector(Extents.X, Extents.Y, Extents.Z);
    Result.Orientation = FQuat::Identity;
    Result.bIsValid = (Result.CornerPoints.Num() == 4);
    
    return Result;
}


//=============================================================================
// 几何中心计算 (Requirements: 4.2)
//=============================================================================

FVector FMAGeometryUtils::ComputeCenterFromActors(const TArray<AActor*>& Actors)
{
    FVector Center = FVector::ZeroVector;
    
    if (Actors.Num() == 0) return Center;
    
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
    
    if (CornerPoints.Num() != 4) return Midpoints;
    
    float Edge01Length = FVector::Dist(CornerPoints[0], CornerPoints[1]);
    float Edge12Length = FVector::Dist(CornerPoints[1], CornerPoints[2]);
    
    if (Edge01Length < Edge12Length)
    {
        FVector Midpoint0 = (CornerPoints[0] + CornerPoints[1]) * 0.5f;
        FVector Midpoint1 = (CornerPoints[2] + CornerPoints[3]) * 0.5f;
        Midpoints.Add(Midpoint0);
        Midpoints.Add(Midpoint1);
    }
    else
    {
        FVector Midpoint0 = (CornerPoints[1] + CornerPoints[2]) * 0.5f;
        FVector Midpoint1 = (CornerPoints[3] + CornerPoints[0]) * 0.5f;
        Midpoints.Add(Midpoint0);
        Midpoints.Add(Midpoint1);
    }
    
    return Midpoints;
}
