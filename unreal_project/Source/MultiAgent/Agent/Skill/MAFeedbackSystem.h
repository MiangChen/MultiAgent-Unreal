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
