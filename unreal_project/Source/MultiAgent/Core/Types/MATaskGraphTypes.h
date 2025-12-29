// MATaskGraphTypes.h
// 任务图数据类型定义 - Task Planner Workbench
// Requirements: 3.1, 9.2

#pragma once

#include "CoreMinimal.h"
#include "MATaskGraphTypes.generated.h"

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMATaskGraph, Log, All);

//=============================================================================
// 前向声明
//=============================================================================
class UTexture2D;

//=============================================================================
// FMARequiredSkill - 所需技能
//=============================================================================

/**
 * 任务所需技能
 * 描述执行任务需要的技能及分配的机器人类型和数量
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMARequiredSkill
{
    GENERATED_BODY()

    /** 技能名称 (如 "navigate_to(building_A_roof)") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString SkillName;

    /** 分配的机器人类型列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    TArray<FString> AssignedRobotType;

    /** 分配的机器人数量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    int32 AssignedRobotCount = 1;

    FMARequiredSkill() {}

    FMARequiredSkill(const FString& InSkillName, const TArray<FString>& InRobotTypes, int32 InCount = 1)
        : SkillName(InSkillName), AssignedRobotType(InRobotTypes), AssignedRobotCount(InCount) {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 对象解析 */
    static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMARequiredSkill& Out);

    /** 相等比较 */
    bool operator==(const FMARequiredSkill& Other) const;
};

//=============================================================================
// FMATaskNodeData - 任务节点数据
//=============================================================================

/**
 * 任务节点数据
 * DAG 中的单个任务节点，包含任务描述、位置、所需技能等
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskNodeData
{
    GENERATED_BODY()

    /** 任务 ID (唯一标识) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString TaskId;

    /** 任务描述 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString Description;

    /** 任务位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString Location;

    /** 所需技能列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    TArray<FMARequiredSkill> RequiredSkills;

    /** 产出列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    TArray<FString> Produces;

    /** 画布位置 (用于渲染，不参与 JSON 序列化) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FVector2D CanvasPosition = FVector2D::ZeroVector;

    FMATaskNodeData() {}

    FMATaskNodeData(const FString& InTaskId, const FString& InDescription, const FString& InLocation)
        : TaskId(InTaskId), Description(InDescription), Location(InLocation) {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 对象解析 */
    static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMATaskNodeData& Out);

    /** 相等比较 */
    bool operator==(const FMATaskNodeData& Other) const;
};

//=============================================================================
// FMATaskEdgeData - 任务边数据
//=============================================================================

/**
 * 任务边数据
 * 表示任务节点之间的依赖关系
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskEdgeData
{
    GENERATED_BODY()

    /** 源节点 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString FromNodeId;

    /** 目标节点 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString ToNodeId;

    /** 边类型: sequential, parallel, conditional */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString EdgeType = TEXT("sequential");

    /** 条件表达式 (仅 conditional 类型使用) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString Condition;

    FMATaskEdgeData() {}

    FMATaskEdgeData(const FString& InFrom, const FString& InTo, const FString& InType = TEXT("sequential"))
        : FromNodeId(InFrom), ToNodeId(InTo), EdgeType(InType) {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 对象解析 */
    static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMATaskEdgeData& Out);

    /** 相等比较 */
    bool operator==(const FMATaskEdgeData& Other) const;
};

//=============================================================================
// FMATaskGraphData - 任务图数据
//=============================================================================

/**
 * 任务图数据
 * 完整的 DAG 数据结构，包含所有节点和边
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskGraphData
{
    GENERATED_BODY()

    /** 任务图描述 (元数据) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString Description;

    /** 节点列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    TArray<FMATaskNodeData> Nodes;

    /** 边列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    TArray<FMATaskEdgeData> Edges;

    FMATaskGraphData() {}

    /** 序列化为格式化的 JSON 字符串 */
    FString ToJson() const;

    /** 从 JSON 字符串解析 */
    static bool FromJson(const FString& Json, FMATaskGraphData& OutData);

    /** 从 JSON 字符串解析，带错误信息 */
    static bool FromJsonWithError(const FString& Json, FMATaskGraphData& OutData, FString& OutError);

    /** 从 response_example.json 格式解析 (包含 meta 和 task_graph 字段) */
    static bool FromResponseJson(const FString& Json, FMATaskGraphData& OutData, FString& OutError);

    /** 查找节点 */
    FMATaskNodeData* FindNode(const FString& TaskId);
    const FMATaskNodeData* FindNode(const FString& TaskId) const;

    /** 检查节点是否存在 */
    bool HasNode(const FString& TaskId) const;

    /** 检查边是否存在 */
    bool HasEdge(const FString& FromId, const FString& ToId) const;

    /** 验证是否为有效 DAG (无环) */
    bool IsValidDAG() const;

    /** 相等比较 */
    bool operator==(const FMATaskGraphData& Other) const;
};

//=============================================================================
// FMANodeTemplate - 节点模板
//=============================================================================

/**
 * 节点模板
 * 用于节点工具栏，提供预定义的节点类型
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMANodeTemplate
{
    GENERATED_BODY()

    /** 模板名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString TemplateName;

    /** 默认描述 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString DefaultDescription;

    /** 默认位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString DefaultLocation;

    /** 图标 (可选) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    UTexture2D* Icon = nullptr;

    FMANodeTemplate() {}

    FMANodeTemplate(const FString& InName, const FString& InDesc, const FString& InLocation)
        : TemplateName(InName), DefaultDescription(InDesc), DefaultLocation(InLocation) {}
};
