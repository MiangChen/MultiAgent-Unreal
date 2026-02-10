// MASkillTemplateRegistry.h
// 技能模板注册表 - 定义每个技能需要检查哪些条件项

#pragma once

#include "CoreMinimal.h"
#include "MAConditionCheckTypes.h"

enum class EMACommand : uint8;
enum class EMAAgentType : uint8;

/**
 * 技能模板注册表
 * 静态注册表类，定义每个 EMACommand 对应的预检查项和运行时检查项。
 * GetTemplate 根据 Command 和 AgentType 返回过滤后的检查模板。
 */
class MULTIAGENT_API FMASkillTemplateRegistry
{
public:
	/**
	 * 获取指定技能和机器人类型的检查模板
	 * 根据 AgentType 过滤环境检查项（仅空中机器人适用，且排除 TakeOff/Land/ReturnHome）
	 */
	static FMASkillCheckTemplate GetTemplate(EMACommand Command, EMAAgentType AgentType);

	/** 判断是否为空中机器人类型 */
	static bool IsAerialRobot(EMAAgentType AgentType);

	/** 判断是否为环境检查豁免技能 (TakeOff, Land, ReturnHome) */
	static bool IsEnvironmentExemptSkill(EMACommand Command);

private:
	/** 基础模板映射表（未按 AgentType 过滤） */
	static TMap<EMACommand, FMASkillCheckTemplate> BaseTemplates;

	/** 初始化基础模板 */
	static void InitBaseTemplates();

	/** 初始化标志 */
	static bool bInitialized;
};
