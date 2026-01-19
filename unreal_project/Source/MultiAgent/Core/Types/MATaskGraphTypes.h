// MATaskGraphTypes.h
// 任务图数据类型定义 - Task Planner Workbench

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
// EMATaskExecutionStatus - 任务执行状态枚举
//=============================================================================

/**
 * 任务执行状态枚举
 * 表示任务节点的当前执行状态
 */
UENUM(BlueprintType)
enum class EMATaskExecutionStatus : uint8
{
    Pending     UMETA(DisplayName = "Pending"),      // 灰色 - 未执行
    InProgress  UMETA(DisplayName = "In Progress"),  // 黄色 - 正在执行
    Completed   UMETA(DisplayName = "Completed"),    // 绿色 - 执行成功
    Failed      UMETA(DisplayName = "Failed")        // 红色 - 执行失败
};

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

    /** 执行状态 (用于渲染，不参与 JSON 序列化) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    EMATaskExecutionStatus Status = EMATaskExecutionStatus::Pending;

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

//=============================================================================
// Skill Allocation Viewer Types
//=============================================================================

/**
 * 技能执行状态枚举
 * 表示技能的当前执行状态
 */
UENUM(BlueprintType)
enum class ESkillExecutionStatus : uint8
{
    Pending     UMETA(DisplayName = "Pending"),
    InProgress  UMETA(DisplayName = "In Progress"),
    Completed   UMETA(DisplayName = "Completed"),
    Failed      UMETA(DisplayName = "Failed")
};

/**
 * 技能分配
 * 表示在特定时间步为特定机器人分配的单个技能
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillAssignment
{
    GENERATED_BODY()

    /** 技能名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString SkillName;

    /** 技能参数 (JSON 字符串格式) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString ParamsJson;

    /** 执行状态 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    ESkillExecutionStatus Status = ESkillExecutionStatus::Pending;

    FMASkillAssignment() {}

    FMASkillAssignment(const FString& InSkillName, const FString& InParamsJson)
        : SkillName(InSkillName), ParamsJson(InParamsJson) {}

    /** 从 JSON 对象解析 */
    static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMASkillAssignment& Out);

    /** 从 JSON 对象解析，带详细错误信息 */
    static bool FromJsonWithError(const TSharedPtr<FJsonObject>& JsonObject, FMASkillAssignment& Out, FString& OutError, const FString& Context);

    /** 序列化为 JSON 对象 */
    TSharedPtr<FJsonObject> ToJsonObject() const;

    /** 获取参数作为 JSON 对象 */
    TSharedPtr<FJsonObject> GetParamsAsJson() const;

    /** 相等比较 */
    bool operator==(const FMASkillAssignment& Other) const;
};

/**
 * 时间步数据
 * 包含特定时间步的所有机器人技能分配
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATimeStepData
{
    GENERATED_BODY()

    /** 机器人 ID -> 技能分配映射 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    TMap<FString, FMASkillAssignment> RobotSkills;

    FMATimeStepData() {}

    /** 从 JSON 对象解析 */
    static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMATimeStepData& Out);

    /** 从 JSON 对象解析，带详细错误信息 */
    static bool FromJsonWithError(const TSharedPtr<FJsonObject>& JsonObject, FMATimeStepData& Out, FString& OutError, int32 TimeStep);

    /** 序列化为 JSON 对象 */
    TSharedPtr<FJsonObject> ToJsonObject() const;

    /** 相等比较 */
    bool operator==(const FMATimeStepData& Other) const;

    /** 获取 JSON 类型名称 (用于错误消息) */
    static FString GetJsonTypeName(EJson Type);
};

/**
 * 技能分配数据
 * 完整的技能分配数据结构，包含所有时间步的技能分配
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillAllocationData
{
    GENERATED_BODY()

    /** 技能分配名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString Name;

    /** 技能分配描述 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString Description;

    /** 原始消息 ID (用于 HITL 响应) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString OriginalMessageId;

    /** 时间步 -> 时间步数据映射 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    TMap<int32, FMATimeStepData> Data;

    FMASkillAllocationData() {}

    /** 从 JSON 字符串解析 */
    static bool FromJson(const FString& JsonString, FMASkillAllocationData& OutData, FString& OutError);

    /** 内部解析函数 (支持 GSI 和 Mock Backend 两种格式) */
    static bool FromJsonInternal(const TSharedPtr<FJsonObject>& RootObject, const TSharedPtr<FJsonObject>* DataObject, FMASkillAllocationData& OutData, FString& OutError);

    /** 序列化为格式化的 JSON 字符串 */
    FString ToJson() const;

    /** 获取所有机器人 ID */
    TArray<FString> GetAllRobotIds() const;

    /** 获取所有时间步 */
    TArray<int32> GetAllTimeSteps() const;

    /** 查找特定技能分配 */
    FMASkillAssignment* FindSkill(int32 TimeStep, const FString& RobotId);
    const FMASkillAssignment* FindSkill(int32 TimeStep, const FString& RobotId) const;

    /** 验证机器人 ID 一致性 */
    bool ValidateRobotIds(TArray<FString>& OutUndefinedRobots) const;

    /** 相等比较 */
    bool operator==(const FMASkillAllocationData& Other) const;
};

/**
 * 技能条渲染数据
 * 用于在甘特图中渲染技能条的数据
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillBarRenderData
{
    GENERATED_BODY()

    /** 时间步 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    int32 TimeStep = 0;

    /** 机器人 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString RobotId;

    /** 技能名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString SkillName;

    /** 技能参数 (JSON 字符串) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString ParamsJson;

    /** 执行状态 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    ESkillExecutionStatus Status = ESkillExecutionStatus::Pending;

    /** 渲染位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FVector2D Position = FVector2D::ZeroVector;

    /** 渲染大小 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FVector2D Size = FVector2D::ZeroVector;

    /** 渲染颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FLinearColor Color = FLinearColor::White;

    /** 是否被选中 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    bool bIsSelected = false;

    FMASkillBarRenderData() {}

    FMASkillBarRenderData(int32 InTimeStep, const FString& InRobotId, const FMASkillAssignment& InSkill)
        : TimeStep(InTimeStep), RobotId(InRobotId), SkillName(InSkill.SkillName), 
          ParamsJson(InSkill.ParamsJson), Status(InSkill.Status) {}
};

//=============================================================================
// Gantt Chart Drag-Drop Types
//=============================================================================

/**
 * 甘特图拖拽状态枚举
 * 表示当前拖拽操作的状态
 */
UENUM(BlueprintType)
enum class EGanttDragState : uint8
{
    Idle        UMETA(DisplayName = "Idle"),        // 无拖拽
    Potential   UMETA(DisplayName = "Potential"),   // 潜在拖拽（鼠标按下但未移动足够距离）
    Dragging    UMETA(DisplayName = "Dragging")     // 拖拽中
};

/**
 * 甘特图拖拽源信息
 * 记录被拖拽技能块的原始位置和数据
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FGanttDragSource
{
    GENERATED_BODY()

    /** 原始时间步 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    int32 TimeStep = -1;

    /** 原始机器人 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FString RobotId;

    /** 技能名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FString SkillName;

    /** 技能参数 (JSON 字符串) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FString ParamsJson;

    /** 执行状态 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    ESkillExecutionStatus Status = ESkillExecutionStatus::Pending;

    /** 原始屏幕位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FVector2D OriginalPosition = FVector2D::ZeroVector;

    FGanttDragSource() {}

    FGanttDragSource(int32 InTimeStep, const FString& InRobotId, const FString& InSkillName,
                     const FString& InParamsJson, ESkillExecutionStatus InStatus, const FVector2D& InPosition)
        : TimeStep(InTimeStep), RobotId(InRobotId), SkillName(InSkillName),
          ParamsJson(InParamsJson), Status(InStatus), OriginalPosition(InPosition) {}

    /** 检查是否有效 */
    bool IsValid() const { return TimeStep >= 0 && !RobotId.IsEmpty(); }

    /** 重置为无效状态 */
    void Reset()
    {
        TimeStep = -1;
        RobotId.Empty();
        SkillName.Empty();
        ParamsJson.Empty();
        Status = ESkillExecutionStatus::Pending;
        OriginalPosition = FVector2D::ZeroVector;
    }
};

/**
 * 甘特图放置目标信息
 * 记录拖拽释放的目标位置和状态
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FGanttDropTarget
{
    GENERATED_BODY()

    /** 目标时间步 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    int32 TimeStep = -1;

    /** 目标机器人 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FString RobotId;

    /** 是否为有效放置目标 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    bool bIsValid = false;

    /** 是否已吸附到槽位 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    bool bIsSnapped = false;

    /** 吸附位置 (槽位中心坐标) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FVector2D SnapPosition = FVector2D::ZeroVector;

    FGanttDropTarget() {}

    FGanttDropTarget(int32 InTimeStep, const FString& InRobotId, bool bInIsValid, 
                     bool bInIsSnapped, const FVector2D& InSnapPosition)
        : TimeStep(InTimeStep), RobotId(InRobotId), bIsValid(bInIsValid),
          bIsSnapped(bInIsSnapped), SnapPosition(InSnapPosition) {}

    /** 检查是否有有效目标 */
    bool HasTarget() const { return TimeStep >= 0 && !RobotId.IsEmpty(); }

    /** 重置为无效状态 */
    void Reset()
    {
        TimeStep = -1;
        RobotId.Empty();
        bIsValid = false;
        bIsSnapped = false;
        SnapPosition = FVector2D::ZeroVector;
    }
};

/**
 * 甘特图拖拽预览信息
 * 用于渲染拖拽过程中的技能块预览
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FGanttDragPreview
{
    GENERATED_BODY()

    /** 当前预览位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FVector2D Position = FVector2D::ZeroVector;

    /** 预览大小 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FVector2D Size = FVector2D::ZeroVector;

    /** 预览颜色 (半透明) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FLinearColor Color = FLinearColor(1.0f, 1.0f, 1.0f, 0.7f);

    /** 技能名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FString SkillName;

    FGanttDragPreview() {}

    FGanttDragPreview(const FVector2D& InPosition, const FVector2D& InSize, 
                      const FLinearColor& InColor, const FString& InSkillName)
        : Position(InPosition), Size(InSize), Color(InColor), SkillName(InSkillName) {}

    /** 重置预览数据 */
    void Reset()
    {
        Position = FVector2D::ZeroVector;
        Size = FVector2D::ZeroVector;
        Color = FLinearColor(1.0f, 1.0f, 1.0f, 0.7f);
        SkillName.Empty();
    }
};
