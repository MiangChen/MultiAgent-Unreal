// MADynamicNodeManager.h
// 动态节点管理模块 - 负责创建和更新动态节点（机器人、环境对象）

#pragma once

#include "CoreMinimal.h"
#include "../MASceneGraphNodeTypes.h"

struct FMAAgentConfigData;
struct FMAEnvironmentObjectConfig;

/**
 * 动态节点管理工具类
 * 
 * 职责:
 * - 创建机器人节点
 * - 创建环境对象节点 (cargo, charging_station, person, vehicle, boat, fire, smoke, wind)
 * - 更新动态节点的位置和状态
 */
class MULTIAGENT_API FMADynamicNodeManager
{
public:
    //=========================================================================
    // 节点创建
    //=========================================================================

    /** 从 Agent 配置创建机器人节点 */
    static FMASceneGraphNode CreateAgentNode(const FMAAgentConfigData& Config);

    /** 从环境对象配置创建节点 */
    static FMASceneGraphNode CreateEnvironmentObjectNode(const FMAEnvironmentObjectConfig& Config);

    //=========================================================================
    // 节点更新
    //=========================================================================

    /** 更新动态节点位置 */
    static bool UpdateNodePosition(FMASceneGraphNode& Node, const FVector& NewPosition);

    /** 更新动态节点旋转 */
    static bool UpdateNodeRotation(FMASceneGraphNode& Node, const FRotator& NewRotation);

    /** 更新动态节点的 Feature (通用 key-value 更新) */
    static bool UpdateNodeFeature(FMASceneGraphNode& Node, const FString& Key, const FString& Value);

    /** 移除动态节点的 Feature */
    static bool RemoveNodeFeature(FMASceneGraphNode& Node, const FString& Key);

    //=========================================================================
    // GUID 绑定相关
    //=========================================================================

    /**
     * 生成节点的 RawJson 字符串
     * 包含 id, guid, properties (type, label, category, is_dynamic, features), shape (type, center)
     * @param Node 节点数据
     * @return JSON 字符串
     */
    static FString GenerateRawJson(const FMASceneGraphNode& Node);

    /**
     * 更新节点的 GUID 并重新生成 RawJson
     * @param Node 节点引用
     * @param NewGuid 新的 GUID
     * @return 是否成功
     */
    static bool UpdateNodeGuid(FMASceneGraphNode& Node, const FString& NewGuid);

    //=========================================================================
    // 辅助方法
    //=========================================================================

    /** 验证位置坐标是否有效 */
    static bool IsValidPosition(const FVector& Position);
};
