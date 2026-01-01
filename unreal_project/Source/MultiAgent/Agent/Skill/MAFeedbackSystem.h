// MAFeedbackSystem.h
// ========== 反馈系统 ==========
// 负责技能执行反馈的生成和模板管理

#pragma once

#include "CoreMinimal.h"
#include "MAFeedbackSystem.generated.h"

// 前向声明
enum class EMACommand : uint8;

// ========== 执行反馈上下文 ==========
USTRUCT(BlueprintType)
struct FMAFeedbackContext
{
    GENERATED_BODY()

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

    // Navigate 附近地标信息 (Requirements: 3.3, 3.4)
    UPROPERTY(BlueprintReadOnly)
    FString NearbyLandmarkLabel;
    
    UPROPERTY(BlueprintReadOnly)
    FString NearbyLandmarkType;
    
    UPROPERTY(BlueprintReadOnly)
    float NearbyLandmarkDistance = 0.f;

    // Follow 技能信息 (Requirements: 5.1, 5.2, 5.3, 5.4, 5.5)
    UPROPERTY(BlueprintReadOnly)
    FString FollowTargetRobotName;
    
    UPROPERTY(BlueprintReadOnly)
    float FollowTargetDistance = 0.f;
    
    UPROPERTY(BlueprintReadOnly)
    bool bFollowTargetFound = false;
    
    UPROPERTY(BlueprintReadOnly)
    FString FollowErrorReason;

    // Charge 技能信息 (Requirements: 7.1, 7.2, 7.3, 7.4)
    UPROPERTY(BlueprintReadOnly)
    FString ChargingStationId;
    
    UPROPERTY(BlueprintReadOnly)
    FVector ChargingStationLocation = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly)
    bool bChargingStationFound = false;
    
    UPROPERTY(BlueprintReadOnly)
    FString ChargeErrorReason;

    // TakeOff 技能信息 (Requirements: 8.1, 8.4)
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

    // Land 技能信息 (Requirements: 8.2, 8.5)
    UPROPERTY(BlueprintReadOnly)
    FVector LandTargetLocation = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly)
    FString LandGroundType;  // 地面类型 (road, intersection, building_roof, etc.)
    
    UPROPERTY(BlueprintReadOnly)
    FString LandNearbyLandmarkLabel;  // 着陆点附近地标
    
    UPROPERTY(BlueprintReadOnly)
    bool bLandLocationSafe = true;  // 着陆位置是否安全

    // ReturnHome 技能信息 (Requirements: 8.3, 8.6)
    UPROPERTY(BlueprintReadOnly)
    FVector HomeLocationFromSceneGraph = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadOnly)
    FString HomeLandmarkLabel;  // 家位置附近地标标签
    
    UPROPERTY(BlueprintReadOnly)
    bool bHomeLocationFromSceneGraph = false;  // 是否从场景图获取家位置
    
    void Reset() { *this = FMAFeedbackContext(); }
};

// ========== 反馈模板管理 ==========
class MULTIAGENT_API FMAFeedbackTemplates
{
public:
    static FMAFeedbackTemplates& Get();
    
    // 生成反馈消息
    FString GenerateMessage(const FString& AgentName, EMACommand Command, bool bSuccess, const FMAFeedbackContext& Context) const;

private:
    FMAFeedbackTemplates();
    
    struct FMessageTemplate
    {
        FString SuccessTemplate;
        FString FailureTemplate;
        FString InProgressTemplate;
    };
    
    TMap<EMACommand, FMessageTemplate> Templates;
    void InitializeTemplates();
};
