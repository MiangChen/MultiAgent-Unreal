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
    FString SearchAreaToken;
    
    UPROPERTY(BlueprintReadOnly)
    float SearchDurationSeconds = 0.f;
    
    UPROPERTY(BlueprintReadOnly)
    FString SearchTargetSpec;

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
    FString FollowTargetName;
    
    UPROPERTY(BlueprintReadOnly)
    float FollowTargetDistance = 0.f;  // UE5 单位 (cm)
    
    UPROPERTY(BlueprintReadOnly)
    bool bFollowTargetFound = false;
    
    UPROPERTY(BlueprintReadOnly)
    FString FollowErrorReason;
    
    UPROPERTY(BlueprintReadOnly)
    FString FollowRobotId;
    
    UPROPERTY(BlueprintReadOnly)
    FString FollowTargetId;
    
    UPROPERTY(BlueprintReadOnly)
    float FollowDurationSeconds = 0.f;
    
    UPROPERTY(BlueprintReadOnly)
    FString FollowTargetSpec;

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
    float TakeOffMinSafeHeight = 0.f;
    
    UPROPERTY(BlueprintReadOnly)
    FString TakeOffNearbyBuildingLabel;
    
    UPROPERTY(BlueprintReadOnly)
    float TakeOffNearbyBuildingHeight = 0.f;
    
    UPROPERTY(BlueprintReadOnly)
    bool bTakeOffHeightAdjusted = false;

    // Land 技能信息
    UPROPERTY(BlueprintReadOnly)
    FVector LandTargetLocation = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly)
    FString LandGroundType;
    
    UPROPERTY(BlueprintReadOnly)
    FString LandNearbyLandmarkLabel;
    
    UPROPERTY(BlueprintReadOnly)
    bool bLandLocationSafe = true;

    // ReturnHome 技能信息
    UPROPERTY(BlueprintReadOnly)
    FVector HomeLocationFromSceneGraph = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly)
    FString HomeLandmarkLabel;
    
    UPROPERTY(BlueprintReadOnly)
    bool bHomeLocationFromSceneGraph = false;

    // TakePhoto 反馈字段
    UPROPERTY(BlueprintReadOnly)
    bool bPhotoTargetFound = false;
    
    UPROPERTY(BlueprintReadOnly)
    FString PhotoTargetName;
    
    UPROPERTY(BlueprintReadOnly)
    FString PhotoTargetId;
    
    UPROPERTY(BlueprintReadOnly)
    float PhotoRobotTargetDistance = -1.f;  // 米
    
    // Broadcast 反馈字段
    UPROPERTY(BlueprintReadOnly)
    FString BroadcastMessage;
    
    UPROPERTY(BlueprintReadOnly)
    bool bBroadcastTargetFound = false;
    
    UPROPERTY(BlueprintReadOnly)
    FString BroadcastTargetName;
    
    UPROPERTY(BlueprintReadOnly)
    float BroadcastRobotTargetDistance = -1.f;  // 米
    
    UPROPERTY(BlueprintReadOnly)
    float BroadcastDurationSeconds = 0.f;
    
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
    TMap<FString, FString> Data;
};

/**
 * 反馈生成器
 */
class MULTIAGENT_API FMAFeedbackGenerator
{
public:
    /**
     * 生成技能执行反馈（统一入口）
     */
    static FMASkillExecutionFeedback Generate(AMACharacter* Agent, EMACommand Command, bool bSuccess, const FString& Message);

    /**
     * 从场景图节点构建 JSON 对象
     */
    static TSharedPtr<FJsonObject> BuildNodeJsonFromSceneGraph(const FMASceneGraphNode& Node);

    /**
     * 从 UE5 场景数据构建 JSON 对象（回退方案）
     */
    static TSharedPtr<FJsonObject> BuildNodeJsonFromUE5Data(
        const FString& Id,
        const FString& Label,
        const FVector& Location,
        const FString& Type,
        const FString& Category = TEXT(""),
        const TMap<FString, FString>& Features = TMap<FString, FString>());

private:
    // 各技能的反馈生成
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
    
    // 辅助方法
    static UMASceneGraphManager* GetSceneGraphManager(AMACharacter* Agent);
    static void AddCommonFieldsToFeedback(FMASkillExecutionFeedback& Feedback, UMASkillComponent* SkillComp, AMACharacter* Agent);
};
