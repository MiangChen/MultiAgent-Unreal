// MAFeedbackGenerator.h
// 反馈生成器 - 统一生成各类技能的执行反馈

#pragma once

#include "CoreMinimal.h"
#include "../MAFeedbackSystem.h"
#include "MAFeedbackGenerator.generated.h"

class AMACharacter;
class UMASkillComponent;
enum class EMACommand : uint8;

// 技能执行反馈结构
USTRUCT(BlueprintType)
struct FMASkillExecutionFeedback
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString AgentId;

    UPROPERTY(BlueprintReadOnly)
    FString SkillName;

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly)
    FString Message;

    UPROPERTY(BlueprintReadOnly)
    TMap<FString, FString> Data;  // 额外数据
};

/**
 * 反馈生成器
 * 
 * 职责:
 * - 统一的反馈生成接口
 * - 根据技能类型和执行结果生成反馈
 */
class MULTIAGENT_API FMAFeedbackGenerator
{
public:
    /**
     * 生成技能执行反馈（统一入口）
     * @param Agent 执行技能的 Agent
     * @param Command 技能类型
     * @param bSuccess 执行是否成功
     * @param Message 执行消息（来自 NotifySkillCompleted）
     * @return 完整的反馈结构
     */
    static FMASkillExecutionFeedback Generate(AMACharacter* Agent, EMACommand Command, bool bSuccess, const FString& Message);

private:
    // 各技能的反馈生成
    static FMASkillExecutionFeedback GenerateNavigateFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static FMASkillExecutionFeedback GenerateSearchFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static FMASkillExecutionFeedback GenerateFollowFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static FMASkillExecutionFeedback GenerateChargeFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static FMASkillExecutionFeedback GeneratePlaceFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static FMASkillExecutionFeedback GenerateTakeOffFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static FMASkillExecutionFeedback GenerateLandFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static FMASkillExecutionFeedback GenerateReturnHomeFeedback(AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static FMASkillExecutionFeedback GenerateIdleFeedback(AMACharacter* Agent, bool bSuccess, const FString& Message);
    
    // 辅助方法
    static FString CommandToSkillName(EMACommand Command);
};
