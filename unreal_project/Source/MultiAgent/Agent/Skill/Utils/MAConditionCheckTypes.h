// MAConditionCheckTypes.h
// 条件检查系统的核心类型定义

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MAConditionCheckTypes.generated.h"

//=============================================================================
// 枚举定义
//=============================================================================

/**
 * 条件检查项枚举
 * 定义所有可能的条件检查类型，分为环境、目标、机器人三大类
 */
UENUM(BlueprintType)
enum class EMAConditionCheckItem : uint8
{
    // 环境检查
    LowVisibility              UMETA(DisplayName = "LowVisibility"),
    StrongWind                 UMETA(DisplayName = "StrongWind"),
    // 目标检查
    TargetExists               UMETA(DisplayName = "TargetExists"),
    TargetNearby               UMETA(DisplayName = "TargetNearby"),
    SurfaceTargetExists        UMETA(DisplayName = "SurfaceTargetExists"),
    SurfaceTargetNearby        UMETA(DisplayName = "SurfaceTargetNearby"),
    HighPriorityTargetDiscovery UMETA(DisplayName = "HighPriorityTargetDiscovery"),
    // 机器人检查
    BatteryLow                 UMETA(DisplayName = "BatteryLow"),
    RobotFault                 UMETA(DisplayName = "RobotFault")
};

/**
 * 突发事件严重程度
 */
UENUM(BlueprintType)
enum class EMAEventSeverity : uint8
{
    Abort      UMETA(DisplayName = "Abort"),       // 立即终止技能列表
    SoftAbort  UMETA(DisplayName = "SoftAbort"),   // 终止技能列表（带建议）
    Info       UMETA(DisplayName = "Info")          // 仅信息，不终止
};


//=============================================================================
// 结构体定义
//=============================================================================

/**
 * 技能检查模板
 * 定义某个技能需要执行哪些预检查项和运行时检查项
 */
USTRUCT(BlueprintType)
struct FMASkillCheckTemplate
{
    GENERATED_BODY()

    /** 预检查项列表 */
    UPROPERTY(BlueprintReadOnly)
    TArray<EMAConditionCheckItem> PrecheckItems;

    /** 运行时检查项列表 */
    UPROPERTY(BlueprintReadOnly)
    TArray<EMAConditionCheckItem> RuntimeCheckItems;
};

/**
 * 事件模板
 * 定义某种突发事件的反馈内容格式
 */
USTRUCT(BlueprintType)
struct FMAEventTemplate
{
    GENERATED_BODY()

    /** 事件类别: environment, target, robot */
    UPROPERTY(BlueprintReadOnly)
    FString Category;

    /** 事件类型: perception_degraded, target_missing, etc. */
    UPROPERTY(BlueprintReadOnly)
    FString Type;

    /** 严重程度 */
    UPROPERTY(BlueprintReadOnly)
    EMAEventSeverity Severity = EMAEventSeverity::Abort;

    /** 消息模板字符串，含 {placeholder} 占位符 */
    UPROPERTY(BlueprintReadOnly)
    FString MessageTemplate;
};

/**
 * 渲染后的事件数据
 * 事件模板经过参数替换后的最终结果
 */
USTRUCT(BlueprintType)
struct FMARenderedEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString Category;

    UPROPERTY(BlueprintReadOnly)
    FString Type;

    UPROPERTY(BlueprintReadOnly)
    EMAEventSeverity Severity = EMAEventSeverity::Info;

    UPROPERTY(BlueprintReadOnly)
    FString Message;

    UPROPERTY(BlueprintReadOnly)
    TMap<FString, FString> Payload;

    /** 是否为中断级别事件 (Abort 或 SoftAbort) */
    bool IsAbort() const
    {
        return Severity == EMAEventSeverity::Abort || Severity == EMAEventSeverity::SoftAbort;
    }

    /** 获取严重程度的字符串表示 */
    static FString SeverityToString(EMAEventSeverity InSeverity)
    {
        switch (InSeverity)
        {
        case EMAEventSeverity::Abort:     return TEXT("abort");
        case EMAEventSeverity::SoftAbort: return TEXT("soft_abort");
        case EMAEventSeverity::Info:      return TEXT("info");
        default:                          return TEXT("unknown");
        }
    }

    /** 序列化为 JSON 对象 */
    TSharedPtr<FJsonObject> ToJson() const
    {
        TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
        JsonObj->SetStringField(TEXT("category"), Category);
        JsonObj->SetStringField(TEXT("type"), Type);
        JsonObj->SetStringField(TEXT("severity"), SeverityToString(Severity));
        JsonObj->SetStringField(TEXT("message"), Message);

        // payload 作为嵌套对象
        TSharedPtr<FJsonObject> PayloadObj = MakeShared<FJsonObject>();
        for (const auto& Pair : Payload)
        {
            PayloadObj->SetStringField(Pair.Key, Pair.Value);
        }
        JsonObj->SetObjectField(TEXT("payload"), PayloadObj);

        return JsonObj;
    }
};

/**
 * 单项检查结果
 */
struct FMACheckResult
{
    /** 是否通过 */
    bool bPassed = true;

    /** 失败时对应的事件模板 key */
    FString EventKey;

    /** 事件渲染参数 */
    TMap<FString, FString> EventParams;

    static FMACheckResult Pass()
    {
        return FMACheckResult{true, FString(), TMap<FString, FString>()};
    }

    static FMACheckResult Fail(const FString& Key, const TMap<FString, FString>& Params)
    {
        return FMACheckResult{false, Key, Params};
    }
};

/**
 * 预检查/运行时检查聚合结果
 */
struct FMAPrecheckResult
{
    /** 是否全部通过 (仅当无 abort/soft_abort 事件时为 true) */
    bool bAllPassed = true;

    /** abort/soft_abort 级别的失败事件 */
    TArray<FMARenderedEvent> FailedEvents;

    /** info 级别的事件 */
    TArray<FMARenderedEvent> InfoEvents;
};
