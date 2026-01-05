// MALocationUtils.h
// 位置计算辅助类 - 负责计算实体的location_label

#pragma once

#include "CoreMinimal.h"

// 前向声明
struct FMASceneGraphNode;

/**
 * 位置计算辅助工具类
 * 
 * 职责:
 * - 判断节点是否为地点类型
 * - 计算最近的地点标签
 * 
 * 参考: GSI/modules/utils/location_utils.py 的 infer_nearest_location 实现
 * 
 */
class MULTIAGENT_API FMALocationUtils
{
public:
    /**
     * 判断节点是否为地点类型
     * 
     * 地点类型包括:
     * - building: 建筑物
     * - intersection: 路口
     * - street_segment / road_segment: 道路段
     * - area: 区域
     * - trans_facility: 交通设施
     * 
     * @param Node 场景图节点
     * @return 是否为地点类型
     * 
     */
    static bool IsLocationNode(const FMASceneGraphNode& Node);

    /**
     * 计算最近的地点标签
     * 
     * 基于几何距离计算最近地点的标签。
     * 参考 GSI 的 location_utils.py 中的 infer_nearest_location 逻辑。
     * 
     * @param AllNodes 所有场景图节点
     * @param Position 实体位置 (世界坐标)
     * @param MaxDistance 最大搜索距离 (默认 5000cm = 50m)
     * @param ExcludeNodeId 要排除的节点ID (可选，用于排除自身)
     * @return 最近地点的标签，如果没有找到则返回空字符串
     * 
     */
    static FString InferNearestLocationLabel(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FVector& Position,
        float MaxDistance = 5000.f,
        const FString& ExcludeNodeId = TEXT(""));

    /**
     * 计算两点之间的2D距离平方 (忽略Z轴)
     * 
     * @param A 第一个点
     * @param B 第二个点
     * @return 2D距离的平方
     */
    static float Distance2DSquared(const FVector& A, const FVector& B);

    /**
     * 获取节点的中心点位置
     * 
     * 对于不同形状类型:
     * - point: 直接返回 Center
     * - polygon/prism: 返回顶点的几何中心
     * - linestring: 返回端点的几何中心
     * 
     * @param Node 场景图节点
     * @return 节点的中心点位置
     */
    static FVector GetNodeCenterPosition(const FMASceneGraphNode& Node);

private:
    /** 地点类型的 Category 列表 */
    static const TArray<FString> LocationCategories;
    
    /** 地点类型的 Type 列表 */
    static const TArray<FString> LocationTypes;
};
