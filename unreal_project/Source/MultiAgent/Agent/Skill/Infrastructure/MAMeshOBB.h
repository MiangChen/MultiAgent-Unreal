// MAMeshOBB.h
// 基于 mesh 顶点 PCA 的真实贴身 OBB 计算工具
//
// 用途：当 Actor 的形状方向已经"焙"进 mesh 顶点（而不是通过 Component 旋转表达）时，
//      Component 的局部坐标系不再反映 mesh 的实际形状方向。这种情况下用
//      Component-Local AABB 当 OBB 会得到一个外接的、近似世界轴对齐的方盒。
//
// 解决方法：对 mesh 顶点云做主成分分析（PCA），求出三条形状主轴：
//   - e1：方差最大方向 = 物体最长边方向（对叶片即叶片长度）
//   - e2：中间方差方向 = 弦向 / 宽度方向
//   - e3：最小方差方向 = 厚度 / 法线方向
//
// 把所有顶点投影到三轴并取 [min, max]，得到真正贴身的 OBB。
//
// 前置条件：StaticMesh 资产必须勾选 Allow CPU Access，否则 cooked build 里读不到顶点。

#pragma once

#include "CoreMinimal.h"

class AActor;

/** Mesh 顶点 PCA 求出的贴身 OBB */
struct MULTIAGENT_API FMAMeshOBB
{
    /** OBB 中心（世界坐标） */
    FVector Center = FVector::ZeroVector;

    /** 三条主轴方向（单位向量，世界坐标）；约定 |Extent[0]| ≥ |Extent[1]| ≥ |Extent[2]| */
    FVector Axis[3] = { FVector::XAxisVector, FVector::YAxisVector, FVector::ZAxisVector };

    /** 三条主轴上的半尺寸（cm）；与 Axis[i] 一一对应 */
    float Extent[3] = { 0.f, 0.f, 0.f };

    /** OBB 是否成功构建（点数太少 / 协方差奇异时为 false） */
    bool bValid = false;
};

/**
 * 根据 Actor 下所有 StaticMeshComponent 的 LOD0 顶点云做 PCA，求贴身 OBB。
 *
 * @param Actor          目标 Actor（会遍历它的所有 UStaticMeshComponent）
 * @param VertexStride   采样间隔；1 表示用所有顶点，N 表示每 N 个取 1 个。建议 4~16
 * @return 贴身 OBB；若 mesh 不可读 / 顶点太少则 bValid=false
 */
MULTIAGENT_API FMAMeshOBB ComputeMeshOBBFromActor(const AActor& Actor, int32 VertexStride = 4);
