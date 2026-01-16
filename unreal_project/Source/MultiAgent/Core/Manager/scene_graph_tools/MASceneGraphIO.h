// MASceneGraphIO.h
// 场景图文件IO模块 - 负责场景图JSON文件的加载、解析和序列化

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MASceneGraphManager.h"

/**
 * 场景图文件IO工具类
 * 
 * 职责:
 * - 加载基础场景图JSON文件
 * - 解析节点数组为FMASceneGraphNode结构
 * - 序列化节点为JSON格式
 * 
 */
class MULTIAGENT_API FMASceneGraphIO
{
public:
    //=========================================================================
    // 文件加载
    //=========================================================================

    /**
     * 加载基础场景图JSON文件
     * 
     * @param FilePath JSON文件的完整路径
     * @param OutData 输出的JSON对象
     * @return 加载是否成功
     * 
     */
    static bool LoadBaseSceneGraph(const FString& FilePath, TSharedPtr<FJsonObject>& OutData);

    /**
     * 保存场景图到JSON文件
     * 
     * @param FilePath JSON文件的完整路径
     * @param Data 要保存的JSON对象
     * @return 保存是否成功
     */
    static bool SaveSceneGraph(const FString& FilePath, const TSharedPtr<FJsonObject>& Data);

    //=========================================================================
    // 节点解析
    //=========================================================================

    /**
     * 解析JSON节点数组为FMASceneGraphNode数组
     * 
     * 支持所有节点类型:
     * - building (prism形状)
     * - trans_facility (road_segment, intersection)
     * - prop (point形状)
     * - robot (动态节点)
     * - pickup_item (动态节点)
     * - charging_station (动态节点)
     * 
     * @param NodesArray JSON节点数组
     * @return 解析后的节点数组
     * 
     */
    static TArray<FMASceneGraphNode> ParseNodes(const TArray<TSharedPtr<FJsonValue>>& NodesArray);

    /**
     * 解析单个JSON节点为FMASceneGraphNode
     * 
     * @param NodeObject JSON节点对象
     * @return 解析后的节点，如果解析失败则返回无效节点
     */
    static FMASceneGraphNode ParseSingleNode(const TSharedPtr<FJsonObject>& NodeObject);

    //=========================================================================
    // 节点序列化
    //=========================================================================

    /**
     * 序列化节点为JSON对象
     * 
     * @param Node 要序列化的节点
     * @return JSON对象，如果序列化失败则返回nullptr
     * 
     */
    static TSharedPtr<FJsonObject> SerializeNode(const FMASceneGraphNode& Node);

    /**
     * 序列化节点为JSON字符串
     * 
     * @param Node 要序列化的节点
     * @param bPrettyPrint 是否格式化输出
     * @return JSON字符串，如果序列化失败则返回空字符串
     */
    static FString SerializeNodeToString(const FMASceneGraphNode& Node, bool bPrettyPrint = true);

    //=========================================================================
    // 几何中心计算 (公开接口，供 MASceneGraphManager 使用)
    //=========================================================================

    /**
     * 计算多边形顶点的几何中心 (从 JSON 数组)
     * 
     * @param Vertices 顶点数组 (每个顶点为 [x, y, z] 的 JSON 数组)
     * @return 几何中心点
     */
    static FVector CalculatePolygonCentroid(const TArray<TSharedPtr<FJsonValue>>& Vertices);

    /**
     * 计算线串端点的几何中心 (从 JSON 数组)
     * 
     * @param Points 端点数组 (每个点为 [x, y, z] 的 JSON 数组)
     * @return 几何中心点
     */
    static FVector CalculateLineStringCentroid(const TArray<TSharedPtr<FJsonValue>>& Points);

private:
    //=========================================================================
    // 内部辅助方法
    //=========================================================================

    /**
     * 从shape对象解析中心点坐标
     * 
     * @param ShapeObject shape JSON对象
     * @param ShapeType 形状类型
     * @return 中心点坐标
     */
    static FVector ParseCenterFromShape(const TSharedPtr<FJsonObject>& ShapeObject, const FString& ShapeType);

    /**
     * 从properties对象解析节点类别
     * 
     * @param PropertiesObject properties JSON对象
     * @return 节点类别字符串
     */
    static FString ParseCategory(const TSharedPtr<FJsonObject>& PropertiesObject);

    /**
     * 从properties对象解析Features映射
     * 
     * @param PropertiesObject properties JSON对象
     * @param OutFeatures 输出的Features映射
     */
    static void ParseFeatures(const TSharedPtr<FJsonObject>& PropertiesObject, TMap<FString, FString>& OutFeatures);

    /**
     * 创建center数组JSON值
     * 
     * @param Center 中心点坐标
     * @return JSON数组
     */
    static TArray<TSharedPtr<FJsonValue>> CreateCenterArray(const FVector& Center);
};
