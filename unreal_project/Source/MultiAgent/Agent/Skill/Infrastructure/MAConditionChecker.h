// MAConditionChecker.h
// 条件检查器 - 执行预检查和运行时检查逻辑

#pragma once

#include "CoreMinimal.h"
#include "Agent/Skill/Domain/MAConditionCheckTypes.h"
#include "Core/Command/Domain/MACommandTypes.h"

class AMACharacter;
class UMASkillComponent;
class UMASceneGraphManager;
enum class EMAAgentType : uint8;

/**
 * 条件检查器
 * 无状态工具类，与 FMAFeedbackGenerator 保持一致的设计风格。
 * 提供预检查和运行时检查的入口方法，以及各类条件检查的静态方法。
 */
class MULTIAGENT_API FMAConditionChecker
{
public:
	/** 距离阈值 (cm)，判断目标是否在机器人附近 */
	static constexpr float DistanceThresholdCm = 2000.f;

	/** 电量最低阈值 (百分比) */
	static constexpr float BatteryMinPercent = 20.f;

	//=========================================================================
	// 聚合入口
	//=========================================================================

	/**
	 * 预检查入口 - 遍历所有适用的预检查项，收集结果
	 */
	static FMAPrecheckResult RunPrecheck(
		AMACharacter* Agent,
		EMACommand Command,
		UMASkillComponent* SkillComp,
		UMASceneGraphManager* SceneGraphMgr);

	/**
	 * 运行时检查入口 - 遍历所有适用的运行时检查项，收集结果
	 */
	static FMAPrecheckResult RunRuntimeCheck(
		AMACharacter* Agent,
		EMACommand Command,
		UMASkillComponent* SkillComp,
		UMASceneGraphManager* SceneGraphMgr);

	//=========================================================================
	// 环境检查
	//=========================================================================

	/** 检查低可见度 - 查询场景图中 "smoke" 节点，判断 Agent 是否在影响半径内 */
	static FMACheckResult CheckLowVisibility(AMACharacter* Agent, UMASceneGraphManager* SceneGraphMgr);

	/** 检查强风 - 查询场景图中 "wind" 节点，判断 Agent 是否在影响半径内 */
	static FMACheckResult CheckStrongWind(AMACharacter* Agent, UMASceneGraphManager* SceneGraphMgr);

	//=========================================================================
	// 目标检查
	//=========================================================================

	/** 检查目标对象是否存在 */
	static FMACheckResult CheckTargetExists(AMACharacter* Agent, UMASkillComponent* SkillComp, EMACommand Command, UMASceneGraphManager* SceneGraphMgr);

	/** 检查目标对象是否在附近 */
	static FMACheckResult CheckTargetNearby(AMACharacter* Agent, UMASkillComponent* SkillComp, EMACommand Command, UMASceneGraphManager* SceneGraphMgr);

	/** 检查放置面是否存在 (Place) - 也用于装货模式下的承载者检查 */
	static FMACheckResult CheckSurfaceTargetExists(AMACharacter* Agent, UMASkillComponent* SkillComp, UMASceneGraphManager* SceneGraphMgr);

	/** 检查放置面是否在附近 (Place) - 也用于装货模式下的承载者检查 */
	static FMACheckResult CheckSurfaceTargetNearby(AMACharacter* Agent, UMASkillComponent* SkillComp, UMASceneGraphManager* SceneGraphMgr);

	/** 检查高优先级目标发现 (Search 运行时) */
	static FMACheckResult CheckHighPriorityTargetDiscovery(AMACharacter* Agent, UMASkillComponent* SkillComp, UMASceneGraphManager* SceneGraphMgr);

	//=========================================================================
	// 机器人检查
	//=========================================================================

	/** 检查电量是否低于阈值 */
	static FMACheckResult CheckBatteryLow(AMACharacter* Agent, UMASkillComponent* SkillComp);

	/** 检查机器人是否故障 - 通过场景图节点的status属性判断 */
	static FMACheckResult CheckRobotFault(AMACharacter* Agent, UMASceneGraphManager* SceneGraphMgr);

	//=========================================================================
	// 辅助方法
	//=========================================================================

	/** 判断是否为空中机器人 */
	static bool IsAerialRobot(EMAAgentType AgentType);

	/** 判断是否为环境检查豁免技能 */
	static bool IsEnvironmentExemptSkill(EMACommand Command);

	/** 根据技能类型从 SkillParams 中提取目标对象 ID */
	static FString GetTargetObjectId(EMACommand Command, UMASkillComponent* SkillComp);

private:
	/**
	 * 通用聚合逻辑 - 遍历检查项列表，执行对应检查，收集结果
	 */
	static FMAPrecheckResult RunChecks(
		const TArray<EMAConditionCheckItem>& Items,
		AMACharacter* Agent,
		EMACommand Command,
		UMASkillComponent* SkillComp,
		UMASceneGraphManager* SceneGraphMgr);

	/** 执行单个检查项 */
	static FMACheckResult ExecuteCheckItem(
		EMAConditionCheckItem Item,
		AMACharacter* Agent,
		EMACommand Command,
		UMASkillComponent* SkillComp,
		UMASceneGraphManager* SceneGraphMgr);

	/**
	 * 环境危险区域检查通用方法
	 * @param NodeType 节点类型 ("smoke" 或 "wind")
	 * @param DefaultRadius 默认影响半径 (cm)
	 * @param EventKey 失败时的事件 key
	 */
	static FMACheckResult CheckEnvironmentHazard(
		AMACharacter* Agent,
		UMASceneGraphManager* SceneGraphMgr,
		const FString& NodeType,
		float DefaultRadius,
		const FString& EventKey);

	/**
	 * 通用对象存在性检查
	 * @param ObjectId 对象 ID
	 * @param EventKey 失败时的事件 key
	 * @param LabelParamKey 事件参数中的标签 key (如 "target_label", "carrier_label")
	 */
	static FMACheckResult CheckObjectExists(
		const FString& ObjectId,
		UMASceneGraphManager* SceneGraphMgr,
		const FString& EventKey,
		const FString& LabelParamKey);

	/**
	 * 通用对象距离检查
	 * @param ObjectId 对象 ID
	 * @param EventKey 失败时的事件 key
	 * @param LabelParamKey 事件参数中的标签 key
	 */
	static FMACheckResult CheckObjectNearby(
		AMACharacter* Agent,
		const FString& ObjectId,
		UMASceneGraphManager* SceneGraphMgr,
		const FString& EventKey,
		const FString& LabelParamKey);
};
