// MAGeometryUtils.cpp
// UI 几何计算工具类实现
// Requirements: 6.1, 6.2, 6.3, 7.1, 7.2, 7.3

#include "MAUIGeometryUtils.h"
#include "GameFramework/Actor.h"

//=============================================================================
// 辅助方法
//=============================================================================

float FMAUIGeometryUtils::CrossProduct2D(const FVector2D& O, const FVector2D& A, const FVector2D& B)
{
    // 计算向量 OA 和 OB 的叉积: (A-O) × (B-O)
    // = (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X)
    return (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X);
}

bool FMAUIGeometryUtils::CompareByPolarAngle(const FVector2D& Pivot, const FVector2D& A, const FVector2D& B)
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

TArray<FVector2D> FMAUIGeometryUtils::CollectBoundingBoxCorners(const TArray<AActor*>& Actors)
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

TArray<FVector2D> FMAUIGeometryUtils::ComputeConvexHull2D(const TArray<FVector2D>& Points)
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

TArray<FVector2D> FMAUIGeometryUtils::SortByNearestNeighbor(const TArray<FVector2D>& Points)
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
