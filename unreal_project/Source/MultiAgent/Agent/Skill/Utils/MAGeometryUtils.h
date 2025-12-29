// MAGeometryUtils.h
// 几何计算辅助工具

#pragma once

#include "CoreMinimal.h"

class MULTIAGENT_API FMAGeometryUtils
{
public:
    // 检查点是否在凸多边形内（2D，忽略Z轴）
    static bool IsPointInPolygon(const FVector& Point, const TArray<FVector>& PolygonVertices);
    
    // 生成割草机式搜索路径
    static TArray<FVector> GenerateLawnmowerPath(const TArray<FVector>& BoundaryVertices, float ScanWidth);
    
    // 计算多边形边界框
    static void GetPolygonBounds(const TArray<FVector>& Vertices, FVector& OutMin, FVector& OutMax);
    
    // 计算多边形中心
    static FVector GetPolygonCenter(const TArray<FVector>& Vertices);
};
