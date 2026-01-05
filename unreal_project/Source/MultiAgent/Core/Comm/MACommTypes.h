// MACommTypes.h
// 通信协议消息类型定义

#pragma once

#include "CoreMinimal.h"
#include "MACommTypes.generated.h"

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMACommTypes, Log, All);

//=============================================================================
// 消息类型枚举
//=============================================================================

/**
 * 通信消息类型枚举
 * 定义仿真端与规划器后端之间的所有消息类型
 */
UENUM(BlueprintType)
enum class EMACommMessageType : uint8
{
    // 出站消息 (仿真端 -> 规划器)
    UIInput         UMETA(DisplayName = "UI Input"),
    ButtonEvent     UMETA(DisplayName = "Button Event"),
    TaskFeedback    UMETA(DisplayName = "Task Feedback"),
    TaskGraphSubmit UMETA(DisplayName = "Task Graph Submit"),
    WorldState      UMETA(DisplayName = "World State"),  // 世界状态响应
    SceneChange     UMETA(DisplayName = "Scene Change"),  // 场景变化消息

    // 入站消息 (规划器 -> 仿真端)
    TaskPlanDAG     UMETA(DisplayName = "Task Plan DAG"),
    WorldModelGraph UMETA(DisplayName = "World Model Graph"),
    SkillList       UMETA(DisplayName = "Skill List"),
    QueryRequest    UMETA(DisplayName = "Query Request"),  // 查询请求

    // 预留扩展
    Custom          UMETA(DisplayName = "Custom")
};

//=============================================================================
// 消息信封结构
//=============================================================================

/**
 * 消息信封 - 统一的消息包装格式
 * 包含消息类型、时间戳、消息ID和负载数据
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAMessageEnvelope
{
    GENERATED_BODY()

    /** 消息类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope")
    EMACommMessageType MessageType = EMACommMessageType::Custom;

    /** 时间戳 (Unix timestamp in milliseconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope")
    int64 Timestamp = 0;

    /** 消息 ID (UUID) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope")
    FString MessageId;

    /** JSON 格式的负载数据 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope")
    FString PayloadJson;

    FMAMessageEnvelope() {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 解析 */
    static bool FromJson(const FString& Json, FMAMessageEnvelope& OutEnvelope);

    /** 生成新的消息 ID (UUID) */
    static FString GenerateMessageId();

    /** 获取当前时间戳 (毫秒) */
    static int64 GetCurrentTimestamp();
};

//=============================================================================
// 出站消息结构 - UI 输入消息
//=============================================================================

/**
 * UI 输入消息
 * 从输入框提交的文本数据，包含输入框标识和内容
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAUIInputMessage
{
    GENERATED_BODY()

    /** 输入源标识 (Widget 名称) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIInput")
    FString InputSourceId;

    /** 输入内容 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIInput")
    FString InputContent;

    FMAUIInputMessage() {}

    FMAUIInputMessage(const FString& InSourceId, const FString& InContent)
        : InputSourceId(InSourceId), InputContent(InContent) {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 解析 */
    static bool FromJson(const FString& Json, FMAUIInputMessage& Out);
};

//=============================================================================
// 出站消息结构 - 按钮事件消息
//=============================================================================

/**
 * 按钮事件消息
 * 按钮点击产生的事件，包含界面名、按钮标识、按钮文字
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAButtonEventMessage
{
    GENERATED_BODY()

    /** 所属界面名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ButtonEvent")
    FString WidgetName;

    /** 按钮标识 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ButtonEvent")
    FString ButtonId;

    /** 按钮显示文字 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ButtonEvent")
    FString ButtonText;

    FMAButtonEventMessage() {}

    FMAButtonEventMessage(const FString& InWidgetName, const FString& InButtonId, const FString& InButtonText)
        : WidgetName(InWidgetName), ButtonId(InButtonId), ButtonText(InButtonText) {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 解析 */
    static bool FromJson(const FString& Json, FMAButtonEventMessage& Out);
};

//=============================================================================
// 出站消息结构 - 任务反馈消息
//=============================================================================

/**
 * 任务反馈消息
 * 任务执行结果反馈，包含状态、耗时、耗能等
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskFeedbackMessage
{
    GENERATED_BODY()

    /** 任务 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskFeedback")
    FString TaskId;

    /** 任务状态: success, timeout, failed, in_progress */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskFeedback")
    FString Status;

    /** 任务耗时 (秒) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskFeedback")
    float DurationSeconds = 0.0f;

    /** 任务耗能 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskFeedback")
    float EnergyConsumed = 0.0f;

    /** 错误信息 (可选，失败时填写) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskFeedback")
    FString ErrorMessage;

    FMATaskFeedbackMessage() {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 解析 */
    static bool FromJson(const FString& Json, FMATaskFeedbackMessage& Out);
};


//=============================================================================
// 入站消息结构 - 任务规划 DAG
//=============================================================================

/**
 * 任务规划节点
 * DAG 中的单个任务节点
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskPlanNode
{
    GENERATED_BODY()

    /** 节点 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    FString NodeId;

    /** 任务类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    FString TaskType;

    /** 任务参数 (键值对) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    TMap<FString, FString> Parameters;

    /** 依赖的节点 ID 列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    TArray<FString> Dependencies;

    FMATaskPlanNode() {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 解析 */
    static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMATaskPlanNode& Out);
};

/**
 * 任务规划边
 * DAG 中的依赖关系边
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskPlanEdge
{
    GENERATED_BODY()

    /** 起始节点 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    FString FromNodeId;

    /** 目标节点 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    FString ToNodeId;

    FMATaskPlanEdge() {}

    FMATaskPlanEdge(const FString& InFrom, const FString& InTo)
        : FromNodeId(InFrom), ToNodeId(InTo) {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 解析 */
    static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMATaskPlanEdge& Out);
};

/**
 * 任务规划 DAG
 * 规划器返回的有向无环图，描述任务执行计划
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskPlanDAG
{
    GENERATED_BODY()

    /** 节点列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    TArray<FMATaskPlanNode> Nodes;

    /** 边列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    TArray<FMATaskPlanEdge> Edges;

    FMATaskPlanDAG() {}

    /** 验证是否为有效 DAG (无环) */
    bool IsValidDAG() const;

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 解析 */
    static bool FromJson(const FString& Json, FMATaskPlanDAG& Out);
};

//=============================================================================
// 入站消息结构 - 世界模型图
//=============================================================================

/**
 * 世界模型实体
 * 世界模型图中的单个实体
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAWorldModelEntity
{
    GENERATED_BODY()

    /** 实体 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    FString EntityId;

    /** 实体类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    FString EntityType;

    /** 实体属性 (键值对) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    TMap<FString, FString> Properties;

    FMAWorldModelEntity() {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 解析 */
    static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMAWorldModelEntity& Out);
};

/**
 * 世界模型关系
 * 世界模型图中实体之间的关系
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAWorldModelRelationship
{
    GENERATED_BODY()

    /** 实体 A 的 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    FString EntityAId;

    /** 实体 B 的 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    FString EntityBId;

    /** 关系类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    FString RelationshipType;

    FMAWorldModelRelationship() {}

    FMAWorldModelRelationship(const FString& InEntityA, const FString& InEntityB, const FString& InType)
        : EntityAId(InEntityA), EntityBId(InEntityB), RelationshipType(InType) {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 解析 */
    static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FMAWorldModelRelationship& Out);
};

/**
 * 世界模型图
 * 规划器返回的无向图，描述当前仿真世界状态
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAWorldModelGraph
{
    GENERATED_BODY()

    /** 实体列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    TArray<FMAWorldModelEntity> Entities;

    /** 关系列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    TArray<FMAWorldModelRelationship> Relationships;

    FMAWorldModelGraph() {}

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 解析 */
    static bool FromJson(const FString& Json, FMAWorldModelGraph& Out);
};

//=============================================================================
// 入站消息结构 - 技能列表 (Python 端发送)
// 格式: { "0": { "AgentID": { "skill": "xxx", "params": {...} } }, "1": {...} }
//=============================================================================

/**
 * 单个技能指令的参数
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillParams_Comm
{
    GENERATED_BODY()

    /** 目标位置 (用于 navigate) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FVector DestPosition = FVector::ZeroVector;

    /** 是否有目标位置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    bool bHasDestPosition = false;

    /** 目标类型 (用于 search) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString GoalType;

    /** 任务 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString TaskId;

    /** 搜索区域 (多边形顶点) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TArray<FVector2D> SearchArea;

    /** 区域标识 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString AreaToken;

    /** 目标标识 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString TargetToken;

    /** 目标特征 (键值对) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TMap<FString, FString> TargetFeatures;

    /** 目标语义标签 JSON (用于 Search 技能)
     * 格式: {"class": "...", "type": "...", "features": {"key": "value", ...}}
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString TargetJson;

    /** 目标实体名称 (用于智能参数调整) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString TargetEntity;

    /** 原始 JSON 参数 (保留完整数据) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString RawParamsJson;

    //=========================================================================
    // Place 技能参数
    //=========================================================================

    /** target 语义标签 JSON (Place 技能源对象) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Place")
    FString Object1Json;

    /** surface_target 语义标签 JSON (Place 技能目标对象) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Place")
    FString Object2Json;

    FMASkillParams_Comm() {}
};

/**
 * 单个 Agent 的技能指令
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAAgentSkillCommand
{
    GENERATED_BODY()

    /** Agent ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString AgentId;

    /** 技能名称: navigate, search, follow, charge, place, take_off, land */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString SkillName;

    /** 技能参数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FMASkillParams_Comm Params;

    FMAAgentSkillCommand() {}
};

/**
 * 单个时间步的技能指令集合
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATimeStepCommands
{
    GENERATED_BODY()

    /** 时间步索引 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 TimeStep = 0;

    /** 该时间步内所有 Agent 的指令 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TArray<FMAAgentSkillCommand> Commands;

    FMATimeStepCommands() {}
};

/**
 * 技能列表消息 (按时间步组织)
 * Python 端发送格式: { "0": { "Agent1": {...}, "Agent2": {...} }, "1": {...} }
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillListMessage
{
    GENERATED_BODY()

    /** 按时间步组织的指令列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillList")
    TArray<FMATimeStepCommands> TimeSteps;

    /** 总时间步数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillList")
    int32 TotalTimeSteps = 0;

    FMASkillListMessage() {}

    /** 从 JSON 解析 */
    static bool FromJson(const FString& Json, FMASkillListMessage& Out);

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 获取指定时间步的指令 */
    const FMATimeStepCommands* GetTimeStep(int32 Step) const;
};

//=============================================================================
// 出站消息结构 - 时间步执行反馈
//=============================================================================

/**
 * 单个技能执行反馈
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillFeedback_Comm
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString AgentId;

    UPROPERTY(BlueprintReadWrite)
    FString SkillName;

    UPROPERTY(BlueprintReadWrite)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadWrite)
    FString Message;

    UPROPERTY(BlueprintReadWrite)
    TMap<FString, FString> Data;

    FMASkillFeedback_Comm() {}

    /** 序列化为 JSON 对象 */
    TSharedPtr<FJsonObject> ToJsonObject() const;
};

/**
 * 时间步执行反馈消息
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATimeStepFeedbackMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 TimeStep = 0;

    UPROPERTY(BlueprintReadWrite)
    TArray<FMASkillFeedback_Comm> Feedbacks;

    FMATimeStepFeedbackMessage() {}

    /** 序列化为 JSON */
    FString ToJson() const;
};

/**
 * 技能列表执行完成反馈消息
 * 当整个技能列表执行完成或被中断时发送
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillListCompletedMessage
{
    GENERATED_BODY()

    /** 是否成功完成所有时间步 */
    UPROPERTY(BlueprintReadWrite)
    bool bCompleted = false;

    /** 是否被中断 */
    UPROPERTY(BlueprintReadWrite)
    bool bInterrupted = false;

    /** 已完成的时间步数 */
    UPROPERTY(BlueprintReadWrite)
    int32 CompletedTimeSteps = 0;

    /** 总时间步数 */
    UPROPERTY(BlueprintReadWrite)
    int32 TotalTimeSteps = 0;

    /** 消息描述 */
    UPROPERTY(BlueprintReadWrite)
    FString Message;

    /** 所有时间步的反馈汇总 */
    UPROPERTY(BlueprintReadWrite)
    TArray<FMATimeStepFeedbackMessage> AllTimeStepFeedbacks;

    FMASkillListCompletedMessage() {}

    /** 序列化为 JSON */
    FString ToJson() const;
};

// 委托声明 - 收到技能列表时广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMASkillListReceived, const FMASkillListMessage&, SkillList);


//=============================================================================
// 场景变化消息类型 (Edit Mode)
//=============================================================================

/**
 * 场景变化消息类型枚举
 * 用于 Edit Mode 中通知后端规划器场景发生的变化
 */
UENUM(BlueprintType)
enum class EMASceneChangeType : uint8
{
    AddNode         UMETA(DisplayName = "Add Node"),        // 添加节点
    DeleteNode      UMETA(DisplayName = "Delete Node"),     // 删除节点
    EditNode        UMETA(DisplayName = "Edit Node"),       // 修改节点
    AddGoal         UMETA(DisplayName = "Add Goal"),        // 添加目标点
    DeleteGoal      UMETA(DisplayName = "Delete Goal"),     // 删除目标点
    AddZone         UMETA(DisplayName = "Add Zone"),        // 添加区域
    DeleteZone      UMETA(DisplayName = "Delete Zone"),     // 删除区域
    AddEdge         UMETA(DisplayName = "Add Edge"),        // 添加边
    EditEdge        UMETA(DisplayName = "Edit Edge")        // 修改边
};

/**
 * 场景变化消息
 * 用于 Edit Mode 中向后端规划器发送场景变化通知
 *
 * 消息格式:
 * - add_node: payload 包含完整的 Node JSON
 * - delete_node: payload 包含 Node ID
 * - edit_node: payload 包含修改后的 Node JSON
 * - add_goal: payload 包含 Goal Node JSON
 * - delete_goal: payload 包含 Goal ID
 * - add_zone: payload 包含 Zone Node JSON
 * - delete_zone: payload 包含 Zone ID
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASceneChangeMessage
{
    GENERATED_BODY()

    /** 变化类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SceneChange")
    EMASceneChangeType ChangeType = EMASceneChangeType::AddNode;

    /** 负载数据 (JSON 格式) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SceneChange")
    FString Payload;

    /** 时间戳 (Unix timestamp in milliseconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SceneChange")
    int64 Timestamp = 0;

    /** 消息 ID (UUID) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SceneChange")
    FString MessageId;

    FMASceneChangeMessage()
    {
        Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
        MessageId = FMAMessageEnvelope::GenerateMessageId();
    }

    FMASceneChangeMessage(EMASceneChangeType InType, const FString& InPayload)
        : ChangeType(InType), Payload(InPayload)
    {
        Timestamp = FMAMessageEnvelope::GetCurrentTimestamp();
        MessageId = FMAMessageEnvelope::GenerateMessageId();
    }

    /** 序列化为 JSON */
    FString ToJson() const;

    /** 从 JSON 解析 */
    static bool FromJson(const FString& Json, FMASceneChangeMessage& Out);

    /** 获取变化类型的字符串表示 */
    static FString ChangeTypeToString(EMASceneChangeType Type);

    /** 从字符串解析变化类型 */
    static EMASceneChangeType StringToChangeType(const FString& TypeStr);
};
