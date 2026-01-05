// MATaskGraphModel.h
// 任务图数据模型 - 管理任务图数据的 CRUD 操作

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "../Core/Types/MATaskGraphTypes.h"
#include "MATaskGraphModel.generated.h"

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMATaskGraphModel, Log, All);

//=============================================================================
// 委托声明
//=============================================================================

/** 数据变更委托 - 当任务图数据发生变化时触发 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnModelDataChanged);

/** 节点变更委托 - 当节点被添加/删除/更新时触发 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeChanged, const FString&, NodeId);

/** 边变更委托 - 当边被添加/删除时触发 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEdgeChanged, const FString&, FromNodeId, const FString&, ToNodeId);

//=============================================================================
// UMATaskGraphModel - 任务图数据模型
//=============================================================================

/**
 * 任务图数据模型
 * 管理任务图数据，提供 CRUD 操作接口和变更通知
 * 维护 OriginalData (不可变) 和 WorkingData (可编辑) 双副本
 */
UCLASS(BlueprintType)
class MULTIAGENT_API UMATaskGraphModel : public UObject
{
    GENERATED_BODY()

public:
    UMATaskGraphModel();

    //=========================================================================
    // 数据加载
    //=========================================================================

    /** 从 JSON 字符串加载数据 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool LoadFromJson(const FString& JsonString);

    /** 从 JSON 字符串加载数据，带错误信息 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool LoadFromJsonWithError(const FString& JsonString, FString& OutError);

    /** 从数据结构加载 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    void LoadFromData(const FMATaskGraphData& Data);

    /** 重置为原始数据 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    void ResetToOriginal();

    /** 清空所有数据 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    void Clear();

    //=========================================================================
    // 数据导出
    //=========================================================================

    /** 导出为格式化的 JSON 字符串 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    FString ToJson() const;

    /** 获取工作数据副本 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    FMATaskGraphData GetWorkingData() const;

    /** 获取原始数据副本 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    FMATaskGraphData GetOriginalData() const;

    //=========================================================================
    // 节点操作
    //=========================================================================

    /** 添加节点 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    void AddNode(const FMATaskNodeData& Node);

    /** 创建并添加新节点 (自动生成唯一 ID) */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    FString CreateNode(const FString& Description, const FString& Location);

    /** 删除节点 (同时删除关联的边) */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool RemoveNode(const FString& NodeId);

    /** 更新节点数据 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool UpdateNode(const FString& NodeId, const FMATaskNodeData& NewData);

    /** 更新节点画布位置 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool UpdateNodePosition(const FString& NodeId, FVector2D NewPosition);

    //=========================================================================
    // 边操作
    //=========================================================================

    /** 添加边 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool AddEdge(const FString& FromId, const FString& ToId, const FString& EdgeType = TEXT("sequential"));

    /** 删除边 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool RemoveEdge(const FString& FromId, const FString& ToId);

    //=========================================================================
    // 查询
    //=========================================================================

    /** 获取所有节点 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    TArray<FMATaskNodeData> GetAllNodes() const;

    /** 获取所有边 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    TArray<FMATaskEdgeData> GetAllEdges() const;

    /** 查找节点 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool FindNode(const FString& NodeId, FMATaskNodeData& OutNode) const;

    /** 检查节点是否存在 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool HasNode(const FString& NodeId) const;

    /** 检查边是否存在 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool HasEdge(const FString& FromId, const FString& ToId) const;

    /** 获取节点数量 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    int32 GetNodeCount() const;

    /** 获取边数量 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    int32 GetEdgeCount() const;

    /** 获取连接到指定节点的所有边 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    TArray<FMATaskEdgeData> GetEdgesForNode(const FString& NodeId) const;

    //=========================================================================
    // 验证
    //=========================================================================

    /** 验证是否为有效 DAG (无环) */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool IsValidDAG() const;

    /** 检查添加边是否会创建环路 */
    UFUNCTION(BlueprintCallable, Category = "TaskGraph|Model")
    bool WouldCreateCycle(const FString& FromId, const FString& ToId) const;

    //=========================================================================
    // 委托
    //=========================================================================

    /** 数据变更委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskGraph|Model")
    FOnModelDataChanged OnDataChanged;

    /** 节点添加委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskGraph|Model")
    FOnNodeChanged OnNodeAdded;

    /** 节点删除委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskGraph|Model")
    FOnNodeChanged OnNodeRemoved;

    /** 节点更新委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskGraph|Model")
    FOnNodeChanged OnNodeUpdated;

    /** 边添加委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskGraph|Model")
    FOnEdgeChanged OnEdgeAdded;

    /** 边删除委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskGraph|Model")
    FOnEdgeChanged OnEdgeRemoved;

protected:
    /** 生成唯一节点 ID */
    FString GenerateUniqueNodeId() const;

    /** 广播数据变更 */
    void BroadcastDataChanged();

private:
    /** 原始数据 (不可变) */
    UPROPERTY()
    FMATaskGraphData OriginalData;

    /** 工作数据 (可编辑) */
    UPROPERTY()
    FMATaskGraphData WorkingData;

    /** 节点 ID 计数器 */
    int32 NodeIdCounter = 0;
};
