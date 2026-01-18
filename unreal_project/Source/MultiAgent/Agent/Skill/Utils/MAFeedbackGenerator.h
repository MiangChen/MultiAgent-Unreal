// MAFeedbackGenerator.h
// 反馈生成器 - 统一生成各类技能的执行反馈

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MAFeedbackGenerator.generated.h"

class AMACharacter;
class UMASkillComponent;
class UMASceneGraphManager;
enum class EMACommand : uint8;
struct FMASceneGraphNode;
struct FMASemanticLabel;

// ========== 执行反馈上下文 ==========
USTRUCT(BlueprintType)
struct FMAFeedbackContext
{
    GENERATED_BODY()

    // 任务 ID (从技能参数传入)
    UPROPERTY(BlueprintReadOnly)
    FString TaskId;

    // 通用
    UPROPERTY(BlueprintReadOnly)
    FVector TargetLocation = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly)
    FString TargetName;

    // 进度
    UPROPERTY(BlueprintReadOnly)
    int32 CurrentStep = 0;
    
    UPROPERTY(BlueprintReadOnly)
    int32 TotalSteps = 0;

    // 能量
    UPROPERTY(BlueprintReadOnly)
    float EnergyBefore = 0.f;
    
    UPROPERTY(BlueprintReadOnly)
    float EnergyAfter = 0.f;

    // 搜索结果
    UPROPERTY(BlueprintReadOnly)
    TArray<FString> FoundObjects;
    
    UPROPERTY(BlueprintReadOnly)
    TMap<FString, FString> ObjectAttributes;
    
    UPROPERTY(BlueprintReadOnly)
    TArray<FVector> FoundLocations;
    
    // 搜索技能扩展字段
    UPROPERTY(BlueprintReadOnly)
    FString SearchAreaToken;  // 搜索区域标识 (如 Building-3)
    
    UPROPERTY(BlueprintReadOnly)
    float SearchDurationSeconds = 0.f;  // 搜索持续时间（秒）
    
    UPROPERTY(BlueprintReadOnly)
    FString SearchTargetSpec;  // 搜索目标规格 (JSON 字符串)

    // Place 结果
    UPROPERTY(BlueprintReadOnly)
    FString PlacedObjectName;
    
    UPROPERTY(BlueprintReadOnly)
    FString PlaceTargetName;
    
    UPROPERTY(BlueprintReadOnly)
    FVector PlaceFinalLocation = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly)
    FString PlaceErrorReason;
    
    UPROPERTY(BlueprintReadOnly)
    FString PlaceCancelledPhase;

    // Navigate 附近地标信息
    UPROPERTY(BlueprintReadOnly)
    FString NearbyLandmarkLabel;
    
    UPROPERTY(BlueprintReadOnly)
    FString NearbyLandmarkType;
    
    UPROPERTY(BlueprintReadOnly)
    float NearbyLandmarkDistance = 0.f;

    // Follow 技能信息
    UPROPERTY(BlueprintReadOnly)
    FString FollowTargetRobotName;
    
    UPROPERTY(BlueprintReadOnly)
    float FollowTargetDistance = 0.f;
    
    UPROPERTY(BlueprintReadOnly)
    bool bFollowTargetFound = false;
    
    UPROPERTY(BlueprintReadOnly)
    FString FollowErrorReason;
    
    UPROPERTY(BlueprintReadOnly)
    FString FollowRobotId;  // 执行 Follow 的机器人 ID
    
    UPROPERTY(BlueprintReadOnly)
    FString FollowTargetId;  // 被跟随目标的 ID
    
    UPROPERTY(BlueprintReadOnly)
    float FollowDurationSeconds = 0.f;  // 跟随持续时间（秒）
    
    UPROPERTY(BlueprintReadOnly)
    FString FollowTargetSpec;  // 跟随目标规格 (JSON 字符串)

    // Charge 技能信息
    UPROPERTY(BlueprintReadOnly)
    FString ChargingStationId;
    
    UPROPERTY(BlueprintReadOnly)
    FVector ChargingStationLocation = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly)
    bool bChargingStationFound = false;
    
    UPROPERTY(BlueprintReadOnly)
    FString ChargeErrorReason;

    // TakeOff 技能信息
    UPROPERTY(BlueprintReadOnly)
    float TakeOffTargetHeight = 0.f;
    
    UPROPERTY(BlueprintReadOnly)
    float TakeOffMinSafeHeight = 0.f;  // 附近建筑物最高点 + 安全距离
    
    UPROPERTY(BlueprintReadOnly)
    FString TakeOffNearbyBuildingLabel;  // 附近最高建筑物标签
    
    UPROPERTY(BlueprintReadOnly)
    float TakeOffNearbyBuildingHeight = 0.f;  // 附近最高建筑物高度
    
    UPROPERTY(BlueprintReadOnly)
    bool bTakeOffHeightAdjusted = false;  // 高度是否被调整

    // Land 技能信息
    UPROPERTY(BlueprintReadOnly)
    FVector LandTargetLocation = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly)
    FString LandGroundType;  // 地面类型 (road, intersection, building_roof, etc.)
    
    UPROPERTY(BlueprintReadOnly)
    FString LandNearbyLandmarkLabel;  // 着陆点附近地标
    
    UPROPERTY(BlueprintReadOnly)
    bool bLandLocationSafe = true;  // 着陆位置是否安全

    // ReturnHome 技能信息
    UPROPERTY(BlueprintReadOnly)
    FVector HomeLocationFromSceneGraph = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly)
    FString HomeLandmarkLabel;  // 家位置附近地标标签
    
    UPROPERTY(BlueprintReadOnly)
    bool bHomeLocationFromSceneGraph = false;  // 是否从场景图获取家位置

    // TakePhoto 反馈字段
    UPROPERTY(BlueprintReadOnly)
    bool bPhotoTargetFound = false;
    
    UPROPERTY(BlueprintReadOnly)
    FString PhotoTargetName;
    
    UPROPERTY(BlueprintReadOnly)
    FString PhotoTargetId;
    
    // Broadcast 反馈字段
    UPROPERTY(BlueprintReadOnly)
    FString BroadcastMessage;
    
    UPROPERTY(BlueprintReadOnly)
    bool bBroadcastTargetFound = false;
    
    UPROPERTY(BlueprintReadOnly)
    FString BroadcastTargetName;
    
    // HandleHazard 反馈字段
    UPROPERTY(BlueprintReadOnly)
    bool bHazardTargetFound = false;
    
    UPROPERTY(BlueprintReadOnly)
    FString HazardTargetName;
    
    UPROPERTY(BlueprintReadOnly)
    FString HazardTargetId;
    
    UPROPERTY(BlueprintReadOnly)
    float HazardHandleDurationSeconds = 0.f;
    
    // Guide 反馈字段
    UPROPERTY(BlueprintReadOnly)
    bool bGuideTargetFound = false;
    
    UPROPERTY(BlueprintReadOnly)
    FString GuideTargetName;
    
    UPROPERTY(BlueprintReadOnly)
    FString GuideTargetId;
    
    UPROPERTY(BlueprintReadOnly)
    FVector GuideDestination = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly)
    float GuideDurationSeconds = 0.f;
    
    void Reset() { *this = FMAFeedbackContext(); }
};

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
 * - 优先使用场景图数据，回退到 UE5 场景查询
 * - 确保反馈包含实体标签和位置
 * - 技能完成后更新场景图状态
 * - 使用场景图节点格式返回发现的目标
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

    //=========================================================================
    // 场景图节点 JSON 构建方法
    //=========================================================================

    /**
     * 从场景图节点构建 JSON 对象
     * 包含 id, properties, shape 字段
     * 
     * @param Node 场景图节点
     * @return JSON 对象，包含完整的节点信息
     * 
     */
    static TSharedPtr<FJsonObject> BuildNodeJsonFromSceneGraph(const FMASceneGraphNode& Node);

    /**
     * 从 UE5 场景数据构建 JSON 对象（场景图查询失败时的回退方案）
     * 
     * @param Id 实体 ID
     * @param Label 实体标签
     * @param Location 实体位置
     * @param Type 实体类型
     * @param Category 实体类别
     * @param Features 实体特征属性
     * @return JSON 对象，包含最小化的节点信息
     * 
     */
    static TSharedPtr<FJsonObject> BuildNodeJsonFromUE5Data(
        const FString& Id,
        const FString& Label,
        const FVector& Location,
        const FString& Type,
        const FString& Category = TEXT(""),
        const TMap<FString, FString>& Features = TMap<FString, FString>());

private:
    // 各技能的反馈生成（填充 Data 和 Message 字段）
    static void GenerateNavigateFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateSearchFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateFollowFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateChargeFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GeneratePlaceFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateTakeOffFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateLandFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateReturnHomeFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateIdleFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, bool bSuccess, const FString& Message);
    static void GenerateTakePhotoFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateBroadcastFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateHandleHazardFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    static void GenerateGuideFeedback(FMASkillExecutionFeedback& Feedback, AMACharacter* Agent, UMASkillComponent* SkillComp, bool bSuccess, const FString& Message);
    
    //=========================================================================
    // 场景图查询辅助方法
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
     */
    static void AddEntityInfoToFeedback(
        FMASkillExecutionFeedback& Feedback,
        const FString& Prefix,
        const FString& Label,
        const FVector& Location,
        const FString& Type);
    
    /**
     * 添加通用字段到反馈数据
     * 包括 task_id 等所有技能共用的字段
     * 
     * @param Feedback 反馈结构
     * @param SkillComp 技能组件（用于获取 FeedbackContext）
     */
    static void AddCommonFieldsToFeedback(
        FMASkillExecutionFeedback& Feedback,
        UMASkillComponent* SkillComp);
};
