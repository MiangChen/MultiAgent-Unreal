// MASceneGraphQuery.h
// 场景图查询模块 - 提供分类查询、语义查询和边界查询接口

#pragma once

#include "CoreMinimal.h"
#include "MASceneGraphManager.h"
#include "MASceneGraphTypes.h"

/**
 * 场景图查询工具类
 * 
 * 职责:
 * - 分类查询: 按节点类别筛选 (建筑物、道路、路口、道具、机器人、可拾取物品、充电站)
 * - 语义查询: 根据语义标签查找匹配节点
 * - 边界查询: 在指定边界内查找节点
 * - 位置查询: 判断点是否在建筑物内、查找最近地标
 * 
 */
class MULTIAGENT_API FMASceneGraphQuery
{
public:
    //=========================================================================
    // 分类查询
    //=========================================================================

    /**
     * 按类别获取节点
     * 
     * @param AllNodes 所有节点数组
     * @param Category 类别名称 (building, trans_facility, prop, robot, pickup_item, charging_station)
     * @return 匹配类别的节点数组
     */
    static TArray<FMASceneGraphNode> GetNodesByCategory(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FString& Category);

    /**
     * 获取所有建筑物节点
     * 
     * @param AllNodes 所有节点数组
     * @return 建筑物节点数组
     */
    static TArray<FMASceneGraphNode> GetAllBuildings(const TArray<FMASceneGraphNode>& AllNodes);

    /**
     * 获取所有道路节点 (road_segment, street_segment)
     * 
     * @param AllNodes 所有节点数组
     * @return 道路节点数组
     */
    static TArray<FMASceneGraphNode> GetAllRoads(const TArray<FMASceneGraphNode>& AllNodes);

    /**
     * 获取所有路口节点
     * 
     * @param AllNodes 所有节点数组
     * @return 路口节点数组
     */
    static TArray<FMASceneGraphNode> GetAllIntersections(const TArray<FMASceneGraphNode>& AllNodes);

    /**
     * 获取所有道具节点
     * 
     * @param AllNodes 所有节点数组
     * @return 道具节点数组
     */
    static TArray<FMASceneGraphNode> GetAllProps(const TArray<FMASceneGraphNode>& AllNodes);

    /**
     * 获取所有机器人节点
     * 
     * @param AllNodes 所有节点数组
     * @return 机器人节点数组
     */
    static TArray<FMASceneGraphNode> GetAllRobots(const TArray<FMASceneGraphNode>& AllNodes);

    /**
     * 获取所有可拾取物品节点
     * 
     * @param AllNodes 所有节点数组
     * @return 可拾取物品节点数组
     */
    static TArray<FMASceneGraphNode> GetAllPickupItems(const TArray<FMASceneGraphNode>& AllNodes);

    /**
     * 获取所有充电站节点
     * 
     * @param AllNodes 所有节点数组
     * @return 充电站节点数组
     */
    static TArray<FMASceneGraphNode> GetAllChargingStations(const TArray<FMASceneGraphNode>& AllNodes);

    //=========================================================================
    // 语义查询
    //=========================================================================

    /**
     * 根据语义标签查找节点
     * 
     * 匹配规则:
     * 1. 如果标签的 Class 非空，则节点的 Category 或 Type 必须匹配
     * 2. 如果标签的 Type 非空，则节点的 Type 或 AgentType 必须匹配
     * 3. 标签的所有 Features 必须在节点中存在且值匹配
     * 
     * @param AllNodes 所有节点数组
     * @param Label 语义标签
     * @return 第一个匹配的节点，如果未找到则返回无效节点
     */
    static FMASceneGraphNode FindNodeByLabel(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FMASemanticLabel& Label);

    /**
     * 查找最近的匹配节点
     * 
     * @param AllNodes 所有节点数组
     * @param Label 语义标签
     * @param FromLocation 起始位置
     * @return 最近的匹配节点，如果未找到则返回无效节点
     */
    static FMASceneGraphNode FindNearestNode(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FMASemanticLabel& Label,
        const FVector& FromLocation);

    /**
     * 检查节点是否匹配语义标签
     * 
     * @param Node 要检查的节点
     * @param Label 语义标签
     * @return 是否匹配
     */
    static bool MatchesLabel(const FMASceneGraphNode& Node, const FMASemanticLabel& Label);

    //=========================================================================
    // 边界查询
    //=========================================================================

    /**
     * 在边界内查找匹配节点
     * 
     * @param AllNodes 所有节点数组
     * @param Label 语义标签 (可为空，表示匹配所有节点)
     * @param BoundaryVertices 边界多边形顶点 (至少3个点)
     * @return 在边界内且匹配标签的节点数组
     */
    static TArray<FMASceneGraphNode> FindNodesInBoundary(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FMASemanticLabel& Label,
        const TArray<FVector>& BoundaryVertices);

    /**
     * 判断点是否在任意建筑物内部
     * 
     * @param AllNodes 所有节点数组
     * @param Point 要检查的点
     * @return 是否在建筑物内部
     */
    static bool IsPointInsideBuilding(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FVector& Point);

    /**
     * 查找最近的地标 (建筑物、路口、道具)
     * 
     * @param AllNodes 所有节点数组
     * @param Location 起始位置
     * @param MaxDistance 最大搜索距离 (默认1000cm)
     * @return 最近的地标节点，如果未找到则返回无效节点
     */
    static FMASceneGraphNode FindNearestLandmark(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FVector& Location,
        float MaxDistance = 1000.f);

    //=========================================================================
    // 辅助查询
    //=========================================================================

    /**
     * 根据ID查找节点
     * 
     * @param AllNodes 所有节点数组
     * @param NodeId 节点ID
     * @return 匹配的节点，如果未找到则返回无效节点
     */
    static FMASceneGraphNode FindNodeById(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FString& NodeId);

    /**
     * 根据Label查找节点
     * 
     * @param AllNodes 所有节点数组
     * @param NodeLabel 节点标签
     * @return 匹配的节点，如果未找到则返回无效节点
     */
    static FMASceneGraphNode FindNodeByLabelString(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FString& NodeLabel);

    /**
     * 查找所有匹配语义标签的节点
     * 
     * @param AllNodes 所有节点数组
     * @param Label 语义标签
     * @return 所有匹配的节点数组
     */
    static TArray<FMASceneGraphNode> FindAllNodesByLabel(
        const TArray<FMASceneGraphNode>& AllNodes,
        const FMASemanticLabel& Label);

    //=========================================================================
    // 几何辅助方法 (公开供其他模块使用)
    //=========================================================================

    /**
     * 判断点是否在2D多边形内部 (忽略Z坐标)
     * 使用射线法 (Ray Casting Algorithm)
     * 
     * @param Point 要检查的点
     * @param PolygonVertices 多边形顶点数组
     * @return 是否在多边形内部
     */
    static bool IsPointInPolygon2D(const FVector& Point, const TArray<FVector>& PolygonVertices);

    /**
     * 从节点的RawJson中提取多边形顶点
     * 
     * @param Node 节点
     * @param OutVertices 输出的顶点数组
     * @return 是否成功提取
     */
    static bool ExtractPolygonVertices(const FMASceneGraphNode& Node, TArray<FVector>& OutVertices);

private:
    //=========================================================================
    // 内部辅助方法
    //=========================================================================

    /**
     * 判断节点是否为地标 (建筑物、路口、道具)
     * 
     * @param Node 节点
     * @return 是否为地标
     */
    static bool IsLandmark(const FMASceneGraphNode& Node);
};
