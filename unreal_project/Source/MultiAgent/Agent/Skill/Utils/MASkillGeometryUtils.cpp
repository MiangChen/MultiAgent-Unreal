// MASkillGeometryUtils.cpp
// 技能层几何计算辅助工具
// 注意: 基础几何函数已统一到 MAGeometryUtils，本文件仅提供技能特定的高级功能

#include "MASkillGeometryUtils.h"
#include "../../../Utils/MAGeometryUtils.h"

TArray<FVector> FMASkillGeometryUtils::GenerateLawnmowerPath(const TArray<FVector>& BoundaryVertices, float ScanWidth)
{
    TArray<FVector> Path;
    if (BoundaryVertices.Num() < 3 || ScanWidth <= 0.f) return Path;
    
    FVector MinBound, MaxBound;
    FMAGeometryUtils::GetPolygonBounds(BoundaryVertices, MinBound, MaxBound);
    
    float Z = FMAGeometryUtils::GetPolygonCenter(BoundaryVertices).Z;
    bool bGoingRight = true;
    
    for (float Y = MinBound.Y; Y <= MaxBound.Y; Y += ScanWidth)
    {
        TArray<FVector> RowPoints;
        
        if (bGoingRight)
        {
            for (float X = MinBound.X; X <= MaxBound.X; X += ScanWidth)
            {
                FVector Point(X, Y, Z);
                if (FMAGeometryUtils::IsPointInPolygon2D(Point, BoundaryVertices))
                {
                    RowPoints.Add(Point);
                }
            }
        }
        else
        {
            for (float X = MaxBound.X; X >= MinBound.X; X -= ScanWidth)
            {
                FVector Point(X, Y, Z);
                if (FMAGeometryUtils::IsPointInPolygon2D(Point, BoundaryVertices))
                {
                    RowPoints.Add(Point);
                }
            }
        }
        
        Path.Append(RowPoints);
        bGoingRight = !bGoingRight;
    }
    
    return Path;
}
