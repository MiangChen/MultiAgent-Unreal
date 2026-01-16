// MADynamicNodeManager.h
// 动态节点管理模块 - 负责创建和更新动态节点（机器人、可拾取物品、充电站）

#pragma once

#include "CoreMinimal.h"
#include "MASceneGraphManager.h"

/**
 * 动态节点管理工具类
 * 
 * 职责:
 * - 从 agents.json 创建机器人节点
 * - 从 environment.json 创建可拾取物品节点和充电站节点
 * - 更新动态节点的位置
 * - 更新可拾取物品的携带状态
 * 
 */
class MULTIAGENT_API FMADynamicNodeManager
{
public:
    //=========================================================================
    // 节点创建 - 从配置文件
    //=========================================================================

    /**
     * 从 agents.json 创建机器人节点
     * 
     * 解析 agents.json 中的机器人配置，为每个机器人实例创建场景图节点。
     * 
     * JSON 格式:
     * {
     *   "agents": [
     *     {
     *       "type": "UAV",
     *       "instances": [
     *         {"label": "UAV_01", "position": [x, y, z], "rotation": [pitch, yaw, roll]}
     *       ]
     *     }
     *   ]
     * }
     * 
     * @param AgentsJsonPath agents.json 文件的完整路径
     * @return 创建的机器人节点数组
     * 
     */
    static TArray<FMASceneGraphNode> CreateRobotNodes(const FString& AgentsJsonPath);

    /**
     * 从 environment.json 创建可拾取物品节点
     * 
     * 解析 environment.json 中的 objects 数组，为每个物品创建场景图节点。
     * 
     * JSON 格式:
     * {
     *   "objects": [
     *     {
     *       "id": "1001",
     *       "label": "RedBox",
     *       "type": "cargo",
     *       "position": [x, y, z],
     *       "features": {"color": "red"}
     *     }
     *   ]
     * }
     * 
     * @param EnvironmentJsonPath environment.json 文件的完整路径
     * @return 创建的可拾取物品节点数组
     * 
     */
    static TArray<FMASceneGraphNode> CreatePickupItemNodes(const FString& EnvironmentJsonPath);

    /**
     * 从 environment.json 创建充电站节点
     * 
     * 解析 environment.json 中的 charging_stations 数组，为每个充电站创建场景图节点。
     * 
     * JSON 格式:
     * {
     *   "charging_stations": [
     *     {
     *       "id": "2001",
     *       "label": "ChargingStation_1",
     *       "position": [x, y, z]
     *     }
     *   ]
     * }
     * 
     * @param EnvironmentJsonPath environment.json 文件的完整路径
     * @return 创建的充电站节点数组
     * 
     */
    static TArray<FMASceneGraphNode> CreateChargingStationNodes(const FString& EnvironmentJsonPath);

    //=========================================================================
    // 节点创建 - 工厂方法
    //=========================================================================

    /**
     * 创建单个机器人节点
     * 
     * @param Id 机器人唯一标识 (如 "5001")
     * @param RobotType 机器人类型 (UAV, UGV, Quadruped, Humanoid)
     * @param Position 世界坐标位置
     * @param Rotation 旋转角度
     * @return 创建的机器人节点
     * 
     */
    static FMASceneGraphNode CreateRobotNode(
        const FString& Id,
        const FString& RobotType,
        const FVector& Position,
        const FRotator& Rotation = FRotator::ZeroRotator);

    /**
     * 创建单个可拾取物品节点
     * 
     * @param Id 物品唯一标识 (数字字符串如 "1001")
     * @param ItemLabel 物品标签 (如 "RedBox")
     * @param ItemType 物品类型 (如 "box")
     * @param Position 世界坐标位置
     * @param Features 特征属性 (color, label 等)
     * @return 创建的可拾取物品节点
     * 
     */
    static FMASceneGraphNode CreatePickupItemNode(
        const FString& Id,
        const FString& ItemLabel,
        const FString& ItemType,
        const FVector& Position,
        const TMap<FString, FString>& Features = TMap<FString, FString>());

    /**
     * 创建单个充电站节点
     * 
     * @param Id 充电站唯一标识 (数字字符串如 "2001")
     * @param StationLabel 充电站标签 (如 "ChargingStation_1")
     * @param Position 世界坐标位置
     * @return 创建的充电站节点
     * 
     */
    static FMASceneGraphNode CreateChargingStationNode(
        const FString& Id,
        const FString& StationLabel,
        const FVector& Position);

    //=========================================================================
    // 节点更新
    //=========================================================================

    /**
     * 更新节点位置
     * 
     * 注意: 此方法直接修改传入的节点引用
     * 
     * @param Node 要更新的节点 (引用)
     * @param NewPosition 新的世界坐标位置
     * @return 更新是否成功 (如果节点无效或位置无效则返回 false)
     * 
     */
    static bool UpdateNodePosition(FMASceneGraphNode& Node, const FVector& NewPosition);

    /**
     * 更新节点旋转
     * 
     * @param Node 要更新的节点 (引用)
     * @param NewRotation 新的旋转角度
     * @return 更新是否成功
     */
    static bool UpdateNodeRotation(FMASceneGraphNode& Node, const FRotator& NewRotation);

    /**
     * 更新可拾取物品的携带状态
     * 
     * 当物品被拾取时，设置 bIsCarried=true 和 CarrierId
     * 当物品被放下时，设置 bIsCarried=false 并清空 CarrierId
     * 
     * @param Node 要更新的节点 (引用)
     * @param bIsCarried 是否被携带
     * @param CarrierId 携带者 ID (机器人 ID)，放下时传空字符串
     * @return 更新是否成功 (如果节点不是 PickupItem 则返回 false)
     * 
     */
    static bool UpdatePickupItemCarrierStatus(
        FMASceneGraphNode& Node,
        bool bIsCarried,
        const FString& CarrierId = TEXT(""));

    //=========================================================================
    // 辅助方法
    //=========================================================================

    /**
     * 验证位置坐标是否有效 (非 NaN, 非 Inf)
     * 
     * @param Position 要验证的位置
     * @return 位置是否有效
     */
    static bool IsValidPosition(const FVector& Position);

    /**
     * 生成机器人标签
     * 
     * 格式: "AgentType-N" (如 "UAV-1", "UGV-1")
     * 
     * @param AgentType 机器人类型
     * @param Index 索引号 (从 1 开始)
     * @return 生成的标签
     */
    static FString GenerateRobotLabel(const FString& AgentType, int32 Index);

private:
    //=========================================================================
    // 内部辅助方法
    //=========================================================================

    /**
     * 加载 JSON 文件
     * 
     * @param FilePath JSON 文件路径
     * @param OutData 输出的 JSON 对象
     * @return 加载是否成功
     */
    static bool LoadJsonFile(const FString& FilePath, TSharedPtr<FJsonObject>& OutData);

    /**
     * 从 JSON 数组解析位置坐标
     * 
     * @param PositionArray 位置数组 [x, y, z]
     * @return 解析后的位置向量
     */
    static FVector ParsePositionFromArray(const TArray<TSharedPtr<FJsonValue>>& PositionArray);

    /**
     * 从 JSON 数组解析旋转角度
     * 
     * @param RotationArray 旋转数组 [pitch, yaw, roll]
     * @return 解析后的旋转角度
     */
    static FRotator ParseRotationFromArray(const TArray<TSharedPtr<FJsonValue>>& RotationArray);

    /**
     * 从 JSON 对象解析 Features 映射
     * 
     * @param FeaturesObject features JSON 对象
     * @return 解析后的 Features 映射
     */
    static TMap<FString, FString> ParseFeaturesFromObject(const TSharedPtr<FJsonObject>& FeaturesObject);
};
