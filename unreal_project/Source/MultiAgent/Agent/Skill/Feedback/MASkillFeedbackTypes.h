// MASkillFeedbackTypes.h
// 技能反馈上下文与执行反馈模型

#pragma once

#include "CoreMinimal.h"
#include "MASkillFeedbackTypes.generated.h"

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
