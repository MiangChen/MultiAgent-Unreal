// MASkillGeometryUtils.cpp

#include "MASkillGeometryUtils.h"

bool FMASkillGeometryUtils::IsPointInPolygon(const FVector& Point, const TArray<FVector>& PolygonVertices)
{
    if (PolygonVertices.Num() < 3) return false;
    
    int32 NumVertices = PolygonVertices.Num();
    bool bInside = false;
    
    for (int32 i = 0, j = NumVertices - 1; i < NumVertices; j = i++)
    {
        const FVector& Vi = PolygonVertices[i];
        const FVector& Vj = PolygonVertices[j];
        
        if (((Vi.Y > Point.Y) != (Vj.Y > Point.Y)) &&
            (Point.X < (Vj.X - Vi.X) * (Point.Y - Vi.Y) / (Vj.Y - Vi.Y) + Vi.X))
        {
            bInside = !bInside;
        }
    }
    
    return bInside;
}

TArray<FVector> FMASkillGeometryUtils::GenerateLawnmowerPath(const TArray<FVector>& BoundaryVertices, float ScanWidth)
{
    TArray<FVector> Path;
    if (BoundaryVertices.Num() < 3 || ScanWidth <= 0.f) return Path;
    
    FVector MinBound, MaxBound;
    GetPolygonBounds(BoundaryVertices, MinBound, MaxBound);
    
    float Z = GetPolygonCenter(BoundaryVertices).Z;
    bool bGoingRight = true;
    
    for (float Y = MinBound.Y; Y <= MaxBound.Y; Y += ScanWidth)
    {
        TArray<FVector> RowPoints;
        
        if (bGoingRight)
        {
            for (float X = MinBound.X; X <= MaxBound.X; X += ScanWidth)
            {
                FVector Point(X, Y, Z);
                if (IsPointInPolygon(Point, BoundaryVertices))
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
                if (IsPointInPolygon(Point, BoundaryVertices))
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

void FMASkillGeometryUtils::GetPolygonBounds(const TArray<FVector>& Vertices, FVector& OutMin, FVector& OutMax)
{
    if (Vertices.Num() == 0)
    {
        OutMin = OutMax = FVector::ZeroVector;
        return;
    }
    
    OutMin = OutMax = Vertices[0];
    
    for (const FVector& V : Vertices)
    {
        OutMin.X = FMath::Min(OutMin.X, V.X);
        OutMin.Y = FMath::Min(OutMin.Y, V.Y);
        OutMin.Z = FMath::Min(OutMin.Z, V.Z);
        OutMax.X = FMath::Max(OutMax.X, V.X);
        OutMax.Y = FMath::Max(OutMax.Y, V.Y);
        OutMax.Z = FMath::Max(OutMax.Z, V.Z);
    }
}

FVector FMASkillGeometryUtils::GetPolygonCenter(const TArray<FVector>& Vertices)
{
    if (Vertices.Num() == 0) return FVector::ZeroVector;
    
    FVector Sum = FVector::ZeroVector;
    for (const FVector& V : Vertices)
    {
        Sum += V;
    }
    return Sum / Vertices.Num();
}
