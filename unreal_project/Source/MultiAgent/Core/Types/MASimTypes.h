// MASimTypes.h
// UI 集成相关的数据类型定义
// 基于 UIRef/SimTypes.h 适配，使用 MA 前缀

#pragma once

#include "CoreMinimal.h"
#include "MATypes.h"
#include "MASimTypes.generated.h"

// 前向声明
class AMACharacter;

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMASimTypes, Log, All);

//=============================================================================
// 结构体定义 - 机器人状态数据
//=============================================================================

/**
 * 机器人状态数据 (用于 UI 显示和通信)
 * 基于 UIRef 的 FRobotData 适配
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMARobotData
{
    GENERATED_BODY()

    /** 机器人唯一ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Robot")
    FString RobotID;

    /** 机器人名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Robot")
    FString RobotName;

    /** 机器人类型 (复用现有 EMAAgentType 枚举) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Robot")
    EMAAgentType RobotType = EMAAgentType::Humanoid;

    /** 当前位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Robot")
    FVector Location = FVector::ZeroVector;

    /** 当前朝向 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Robot")
    FRotator Rotation = FRotator::ZeroRotator;

    /** 电量 (0.0 - 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Robot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Battery = 1.0f;

    /** 显示颜色 (用于UI标识) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Robot")
    FLinearColor DisplayColor = FLinearColor::White;

    /** 关联的 Character 引用 (运行时设置) */
    UPROPERTY(BlueprintReadOnly, Category = "Robot")
    TWeakObjectPtr<AMACharacter> CharacterRef;

    FMARobotData() {}

    /** 从 AMACharacter 构造 */
    explicit FMARobotData(const AMACharacter* Character);
};

//=============================================================================
// 结构体定义 - 规划器响应
//=============================================================================

/**
 * 规划器响应数据 (简化版，用于 UI 显示)
 * 基于 UIRef 的 FPlannerResponse 适配
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAPlannerResponse
{
    GENERATED_BODY()

    /** 是否成功 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Response")
    bool bSuccess = true;

    /** 消息内容 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Response")
    FString Message;

    /** 规划结果文本 (用于显示给用户) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Response")
    FString PlanText;

    FMAPlannerResponse() {}

    /** 便捷构造函数 - 成功响应 */
    static FMAPlannerResponse Success(const FString& InMessage, const FString& InPlanText = TEXT(""))
    {
        FMAPlannerResponse Response;
        Response.bSuccess = true;
        Response.Message = InMessage;
        Response.PlanText = InPlanText;
        return Response;
    }

    /** 便捷构造函数 - 失败响应 */
    static FMAPlannerResponse Failure(const FString& InMessage)
    {
        FMAPlannerResponse Response;
        Response.bSuccess = false;
        Response.Message = InMessage;
        return Response;
    }
};

//=============================================================================
// 委托定义
//=============================================================================

/**
 * 规划器响应委托
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMAPlannerResponse, const FMAPlannerResponse&, Response);

// 前向声明入站消息类型（定义在 MACommTypes.h）
struct FMATaskPlanDAG;
struct FMAWorldModelGraph;

/**
 * 任务规划 DAG 接收委托
 * 当从规划器收到任务规划 DAG 时广播
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMATaskPlanReceived, const FMATaskPlanDAG&, TaskPlan);

/**
 * 世界模型图接收委托
 * 当从规划器收到世界模型图时广播
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMAWorldModelReceived, const FMAWorldModelGraph&, WorldModel);
