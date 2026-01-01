// MAFeedbackGenerator.h
// 反馈生成器 - 统一生成各类技能的执行反馈
// Requirements: 9.1, 9.2, 9.3, 9.4, 9.5

#pragma once

#include "CoreMinimal.h"
#include "../MAFeedbackSystem.h"
#include "MAFeedbackGenerator.generated.h"

class AMACharacter;
class UMASkillComponent;
class UMASceneGraphManager;
enum class EMACommand : uint8;
struct FMASceneGraphNode;
struct FMASemanticLabel;

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
 * - 优先使用场景图数据，回退到 UE5 场景查询 (Requirements: 9.1, 9.2)
 * - 确保反馈包含实体标签和位置 (Requirements: 9.3, 9.4)
 * - 技能完成后更新场景图状态 (Requirements: 9.5)
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
    
    //=========================================================================
    // 辅助方法
    //=========================================================================
    
    /** 命令类型转技能名称 */
    static FString CommandToSkillName(EMACommand Command);
    
    //=========================================================================
    // 场景图查询辅助方法 (Requirements: 9.1, 9.2)
    //=========================================================================
    
    /**
     * 获取场景图管理器
     * @param Agent Agent 指针
     * @return 场景图管理器指针，如果获取失败则返回 nullptr
     */
    static UMASceneGraphManager* GetSceneGraphManager(AMACharacter* Agent);
    
    /**
     * 从场景图获取实体信息，带回退机制
     * 优先使用场景图数据，如果未找到则回退到 UE5 场景查询
     * 
     * @param Agent Agent 指针
     * @param EntityId 实体 ID
     * @param OutLabel 输出的实体标签
     * @param OutLocation 输出的实体位置
     * @param OutType 输出的实体类型
     * @return 是否成功获取实体信息
     * 
     * Requirements: 9.1, 9.2
     */
    static bool GetEntityInfoWithFallback(
        AMACharacter* Agent,
        const FString& EntityId,
        FString& OutLabel,
        FVector& OutLocation,
        FString& OutType);
    
    /**
     * 从场景图获取机器人信息
     * 
     * @param Agent Agent 指针
     * @param RobotId 机器人 ID
     * @param OutLabel 输出的机器人标签
     * @param OutLocation 输出的机器人位置
     * @param OutAgentType 输出的机器人类型 (UAV, UGV, etc.)
     * @return 是否成功获取机器人信息
     * 
     * Requirements: 9.1, 9.2
     */
    static bool GetRobotInfoFromSceneGraph(
        AMACharacter* Agent,
        const FString& RobotId,
        FString& OutLabel,
        FVector& OutLocation,
        FString& OutAgentType);
    
    /**
     * 从场景图获取可拾取物品信息
     * 
     * @param Agent Agent 指针
     * @param ItemId 物品 ID
     * @param OutLabel 输出的物品标签
     * @param OutLocation 输出的物品位置
     * @param OutFeatures 输出的物品特征
     * @return 是否成功获取物品信息
     * 
     * Requirements: 9.1, 9.2
     */
    static bool GetPickupItemInfoFromSceneGraph(
        AMACharacter* Agent,
        const FString& ItemId,
        FString& OutLabel,
        FVector& OutLocation,
        TMap<FString, FString>& OutFeatures);
    
    /**
     * 从场景图获取充电站信息
     * 
     * @param Agent Agent 指针
     * @param StationId 充电站 ID
     * @param OutLabel 输出的充电站标签
     * @param OutLocation 输出的充电站位置
     * @return 是否成功获取充电站信息
     * 
     * Requirements: 9.1, 9.2
     */
    static bool GetChargingStationInfoFromSceneGraph(
        AMACharacter* Agent,
        const FString& StationId,
        FString& OutLabel,
        FVector& OutLocation);
    
    /**
     * 添加实体信息到反馈数据
     * 统一格式添加实体标签和位置
     * 
     * @param Feedback 反馈结构
     * @param Prefix 数据键前缀 (如 "object", "target")
     * @param Label 实体标签
     * @param Location 实体位置
     * @param Type 实体类型
     * 
     * Requirements: 9.3, 9.4
     */
    static void AddEntityInfoToFeedback(
        FMASkillExecutionFeedback& Feedback,
        const FString& Prefix,
        const FString& Label,
        const FVector& Location,
        const FString& Type);
    
    //=========================================================================
    // 场景图状态更新 (Requirements: 9.5)
    //=========================================================================
    
    /**
     * 更新场景图中的实体位置
     * 技能完成后调用，同步位置变化
     * 
     * @param Agent Agent 指针
     * @param EntityId 实体 ID
     * @param NewLocation 新位置
     * @param bIsRobot 是否为机器人
     * 
     * Requirements: 9.5
     */
    static void UpdateSceneGraphEntityPosition(
        AMACharacter* Agent,
        const FString& EntityId,
        const FVector& NewLocation,
        bool bIsRobot);
    
    /**
     * 更新场景图中的物品携带状态
     * Place 技能完成后调用
     * 
     * @param Agent Agent 指针
     * @param ItemId 物品 ID
     * @param bIsCarried 是否被携带
     * @param CarrierId 携带者 ID
     * 
     * Requirements: 9.5
     */
    static void UpdateSceneGraphItemCarrierStatus(
        AMACharacter* Agent,
        const FString& ItemId,
        bool bIsCarried,
        const FString& CarrierId);
};
