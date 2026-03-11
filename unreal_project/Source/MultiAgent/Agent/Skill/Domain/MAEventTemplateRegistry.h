// MAEventTemplateRegistry.h
// 突发事件模板注册表 - 定义每种突发事件的反馈内容格式

#pragma once

#include "CoreMinimal.h"
#include "MAConditionCheckTypes.h"

/**
 * 突发事件模板注册表
 * 静态注册表类，定义每种突发事件的 category、type、severity 和消息模板。
 * RenderEvent 根据 EventKey 和参数 TMap 渲染出最终的 FMARenderedEvent。
 */
class MULTIAGENT_API FMAEventTemplateRegistry
{
public:
	/**
	 * 渲染事件：根据 EventKey 查找模板，用 Params 替换消息中的 {placeholder} 占位符
	 * @param EventKey  事件模板 key（如 "AREA_LOW_VISIBILITY"）
	 * @param Params    占位符替换参数（key 不含花括号）
	 * @return 渲染后的事件数据；若 EventKey 未注册则返回 category="unknown" 的默认事件
	 */
	static FMARenderedEvent RenderEvent(const FString& EventKey, const TMap<FString, FString>& Params);

	/**
	 * 检查指定 EventKey 是否已注册
	 */
	static bool HasTemplate(const FString& EventKey);

private:
	/** 事件模板映射表 */
	static TMap<FString, FMAEventTemplate> Templates;

	/** 初始化所有事件模板 */
	static void InitTemplates();

	/** 初始化标志 */
	static bool bInitialized;

	/** 替换消息模板中的 {placeholder} 占位符 */
	static FString SubstitutePlaceholders(const FString& MessageTemplate, const TMap<FString, FString>& Params);
};
