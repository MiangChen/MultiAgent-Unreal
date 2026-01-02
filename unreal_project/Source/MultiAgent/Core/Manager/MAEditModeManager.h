// MAEditModeManager.h
// Edit Mode 管理器 - 负责临时场景图的生命周期和所有编辑操作
// 
// Edit Mode 用于模拟任务执行过程中发生的"新情况"（动态变化）
// 与 Modify Mode（持久化修改源场景图文件）不同，Edit Mode 的所有操作仅针对临时场景图文件进行
//
// Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 3.1, 3.2, 3.4, 3.5, 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7
// Requirements: 6.4, 7.4, 8.4, 9.6, 10.7, 11.1
// Requirements: 14.1, 14.6, 15.2, 15.6, 16.1-16.6, 17.2, 17.3

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonObject.h"
#include "../Comm/MACommTypes.h"
#include "MAEditModeManager.generated.h"

class AMAPointOfInterest;
class AMAZoneActor;
class AMAGoalActor;

// 日志类别
DECLARE_LOG_CATEGORY_EXTERN(LogMAEditMode, Log, All);

//=============================================================================
// 委托声明
//=============================================================================

/** 选择变化委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEditModeSelectionChanged);

/** 临时场景图变化委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTempSceneGraphChanged);

/**
 * Edit Mode 管理器
 * 
 * 职责:
 * - 临时场景图文件的创建、加载、保存和删除
 * - POI (Point of Interest) 的创建和管理
 * - 对象选择管理 (Actor 单选、POI 多选、互斥选择)
 * - Node 操作 (添加、删除、修改)
 * - Zone/Goal Actor 可视化管理
 * - 后端通信 (场景变化通知)
 * 
 * Requirements: 1.1, 1.2, 1.3, 1.4, 1.5
 */
UCLASS()
class MULTIAGENT_API UMAEditModeManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 生命周期
    //=========================================================================
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    //=========================================================================
    // 临时场景图管理
    // Requirements: 1.1, 1.2, 1.3, 1.4, 1.5
    //=========================================================================
    
    /**
     * 创建临时场景图 (从源文件复制)
     * Requirements: 1.1, 1.5
     * 
     * @return 创建是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode")
    bool CreateTempSceneGraph();
    
    /**
     * 删除临时场景图
     * Requirements: 1.4
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode")
    void DeleteTempSceneGraph();
    
    /**
     * 获取临时场景图文件路径
     * Requirements: 1.2
     * 
     * @return 临时文件路径
     */
    UFUNCTION(BlueprintPure, Category = "EditMode")
    FString GetTempSceneGraphPath() const;
    
    /**
     * 检查 Edit Mode 是否可用
     * Requirements: 1.3
     * 
     * @return Edit Mode 是否可用
     */
    UFUNCTION(BlueprintPure, Category = "EditMode")
    bool IsEditModeAvailable() const { return bEditModeAvailable; }

    /**
     * 获取临时场景图 JSON 内容
     * 
     * @return JSON 字符串
     */
    UFUNCTION(BlueprintPure, Category = "EditMode")
    FString GetTempSceneGraphJson() const;

    //=========================================================================
    // POI 管理
    // Requirements: 3.1, 3.2, 3.4, 3.5
    //=========================================================================
    
    /**
     * 在指定位置创建 POI
     * Requirements: 3.1, 3.2
     * 
     * @param WorldLocation 世界坐标
     * @return 创建的 POI Actor，失败返回 nullptr
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|POI")
    AMAPointOfInterest* CreatePOI(const FVector& WorldLocation);
    
    /**
     * 销毁指定 POI
     * 
     * @param POI 要销毁的 POI
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|POI")
    void DestroyPOI(AMAPointOfInterest* POI);
    
    /**
     * 销毁所有 POI
     * Requirements: 3.5
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|POI")
    void DestroyAllPOIs();
    
    /**
     * 获取所有 POI
     * 
     * @return POI 数组
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|POI")
    TArray<AMAPointOfInterest*> GetAllPOIs() const { return POIs; }

    //=========================================================================
    // 选择管理
    // Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7
    //=========================================================================
    
    /**
     * 选择 Actor (单选，自动取消之前选择)
     * Requirements: 4.1, 4.5, 4.6
     * 
     * @param Actor 要选择的 Actor
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Selection")
    void SelectActor(AActor* Actor);
    
    /**
     * 选择 POI (多选)
     * Requirements: 4.2, 4.4, 4.6
     * 
     * @param POI 要选择的 POI
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Selection")
    void SelectPOI(AMAPointOfInterest* POI);
    
    /**
     * 从选择集合移除对象
     * Requirements: 4.7
     * 
     * @param Object 要移除的对象
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Selection")
    void DeselectObject(UObject* Object);
    
    /**
     * 清除所有选择
     * Requirements: 4.3
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Selection")
    void ClearSelection();
    
    /**
     * 获取选中的 Actor (单选)
     * 
     * @return 选中的 Actor，未选中返回 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Selection")
    AActor* GetSelectedActor() const { return SelectedActor; }
    
    /**
     * 获取选中的 POI 列表
     * 
     * @return 选中的 POI 数组
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Selection")
    TArray<AMAPointOfInterest*> GetSelectedPOIs() const { return SelectedPOIs; }
    
    /**
     * 检查是否选中了 Actor
     * 
     * @return 是否有选中的 Actor
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Selection")
    bool HasSelectedActor() const { return SelectedActor != nullptr; }
    
    /**
     * 检查是否选中了 POI
     * 
     * @return 是否有选中的 POI
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Selection")
    bool HasSelectedPOIs() const { return SelectedPOIs.Num() > 0; }


    //=========================================================================
    // Node 操作 (将在后续任务中实现)
    //=========================================================================
    
    /**
     * 添加 Node 到临时场景图
     * 
     * @param NodeJson Node 的 JSON 字符串
     * @param OutError 错误信息
     * @return 添加是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Node")
    bool AddNode(const FString& NodeJson, FString& OutError);
    
    /**
     * 删除 Node
     * 
     * @param NodeId Node ID
     * @param OutError 错误信息
     * @return 删除是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Node")
    bool DeleteNode(const FString& NodeId, FString& OutError);
    
    /**
     * 修改 Node
     * 
     * @param NodeId Node ID
     * @param NewNodeJson 新的 Node JSON
     * @param OutError 错误信息
     * @return 修改是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Node")
    bool EditNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError);
    
    /**
     * 创建 Goal Node
     * 
     * @param Location 位置
     * @param Description 描述
     * @param OutError 错误信息
     * @return 创建是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Node")
    bool CreateGoal(const FVector& Location, const FString& Description, FString& OutError);
    
    /**
     * 创建 Zone Node
     * 
     * @param Vertices 顶点数组
     * @param Description 描述
     * @param OutError 错误信息
     * @return 创建是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Node")
    bool CreateZone(const TArray<FVector>& Vertices, const FString& Description, FString& OutError);

    //=========================================================================
    // 后端通信
    // Requirements: 6.4, 7.4, 8.4, 9.6, 10.7, 11.1
    //=========================================================================
    
    /**
     * 发送场景变化消息到后端规划器
     * Requirements: 11.1
     * 
     * @param ChangeType 变化类型字符串 (add_node, delete_node, edit_node, add_goal, add_zone 等)
     * @param Payload 负载数据 (JSON 格式)
     */
    void SendSceneChangeMessage(const FString& ChangeType, const FString& Payload);
    
    /**
     * 发送场景变化消息 (使用枚举类型)
     * Requirements: 11.1
     * 
     * @param ChangeType 变化类型枚举
     * @param Payload 负载数据 (JSON 格式)
     */
    void SendSceneChangeMessageByType(EMASceneChangeType ChangeType, const FString& Payload);

    //=========================================================================
    // Zone/Goal Actor 管理
    // Requirements: 14.1, 14.6, 15.2, 15.6
    //=========================================================================
    
    /**
     * 为 Zone Node 创建可视化 Actor
     * Requirements: 14.1
     * 
     * @param NodeId Node ID
     * @param Vertices 顶点数组
     * @return 创建的 Zone Actor
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Zone")
    AMAZoneActor* CreateZoneActor(const FString& NodeId, const TArray<FVector>& Vertices);
    
    /**
     * 销毁 Zone Actor
     * 
     * @param NodeId Node ID
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Zone")
    void DestroyZoneActor(const FString& NodeId);
    
    /**
     * 销毁所有 Zone Actor
     * Requirements: 14.6
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Zone")
    void DestroyAllZoneActors();
    
    /**
     * 根据 Node ID 获取 Zone Actor
     * 
     * @param NodeId Node ID
     * @return Zone Actor，未找到返回 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Zone")
    AMAZoneActor* GetZoneActorByNodeId(const FString& NodeId) const;
    
    /**
     * 为 Goal Node 创建可视化 Actor
     * Requirements: 15.2
     * 
     * @param NodeId Node ID
     * @param Location 位置
     * @param Description 描述
     * @return 创建的 Goal Actor
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Goal")
    AMAGoalActor* CreateGoalActor(const FString& NodeId, const FVector& Location, const FString& Description);
    
    /**
     * 销毁 Goal Actor
     * 
     * @param NodeId Node ID
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Goal")
    void DestroyGoalActor(const FString& NodeId);
    
    /**
     * 销毁所有 Goal Actor
     * Requirements: 15.6
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Goal")
    void DestroyAllGoalActors();
    
    /**
     * 根据 Node ID 获取 Goal Actor
     * 
     * @param NodeId Node ID
     * @return Goal Actor，未找到返回 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Goal")
    AMAGoalActor* GetGoalActorByNodeId(const FString& NodeId) const;


    //=========================================================================
    // 设为 Goal 功能
    // Requirements: 16.1, 16.2, 16.3, 16.4, 16.5, 16.6
    //=========================================================================
    
    /**
     * 将 Node 设为 Goal (添加 is_goal: true)
     * Requirements: 16.2, 16.3, 16.4
     * 
     * @param NodeId Node ID
     * @param OutError 错误信息
     * @return 是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Goal")
    bool SetNodeAsGoal(const FString& NodeId, FString& OutError);
    
    /**
     * 取消 Node 的 Goal 状态 (移除 is_goal)
     * Requirements: 16.6
     * 
     * @param NodeId Node ID
     * @param OutError 错误信息
     * @return 是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Goal")
    bool UnsetNodeAsGoal(const FString& NodeId, FString& OutError);
    
    /**
     * 检查 Node 是否为 Goal
     * Requirements: 16.5
     * 
     * @param NodeId Node ID
     * @return 是否为 Goal
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Goal")
    bool IsNodeGoal(const FString& NodeId) const;

    //=========================================================================
    // 列表查询
    // Requirements: 17.2, 17.3
    //=========================================================================
    
    /**
     * 获取所有 Goal Node ID 列表
     * Requirements: 17.2
     * 
     * @return Goal Node ID 数组
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Query")
    TArray<FString> GetAllGoalNodeIds() const;
    
    /**
     * 获取所有 Zone Node ID 列表
     * Requirements: 17.3
     * 
     * @return Zone Node ID 数组
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Query")
    TArray<FString> GetAllZoneNodeIds() const;
    
    /**
     * 根据 Node ID 获取 Node 标签
     * 
     * @param NodeId Node ID
     * @return Node 标签
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Query")
    FString GetNodeLabel(const FString& NodeId) const;

    /**
     * 根据 GUID 查找场景中的 Actor
     * 
     * @param Guid Actor 的 GUID
     * @return 找到的 Actor，未找到返回 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Query")
    AActor* FindActorByGuid(const FString& Guid) const;

    /**
     * 根据 Actor GUID 查找临时场景图中的 Node ID 列表
     * 
     * @param ActorGuid Actor 的 GUID
     * @return 包含该 GUID 的 Node ID 数组
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Query")
    TArray<FString> FindNodeIdsByGuid(const FString& ActorGuid) const;

    /**
     * 根据 ID 查找 Node JSON (公共接口)
     * 
     * @param NodeId Node ID
     * @return Node JSON 字符串，未找到返回空字符串
     */
    UFUNCTION(BlueprintPure, Category = "EditMode|Query")
    FString GetNodeJsonById(const FString& NodeId) const;

    //=========================================================================
    // 委托
    //=========================================================================
    
    /** 选择变化委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEditModeSelectionChanged OnSelectionChanged;
    
    /** 临时场景图变化委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnTempSceneGraphChanged OnTempSceneGraphChanged;

private:
    //=========================================================================
    // 内部状态
    //=========================================================================
    
    /** 临时场景图文件路径 */
    FString TempSceneGraphPath;
    
    /** 临时场景图数据 */
    TSharedPtr<FJsonObject> TempSceneGraphData;
    
    /** POI 列表 */
    UPROPERTY()
    TArray<AMAPointOfInterest*> POIs;
    
    /** 选中的 Actor (单选) */
    UPROPERTY()
    AActor* SelectedActor = nullptr;
    
    /** 选中的 POI 列表 (多选) */
    UPROPERTY()
    TArray<AMAPointOfInterest*> SelectedPOIs;
    
    /** Edit Mode 是否可用 */
    bool bEditModeAvailable = false;
    
    /** 下一个可用的 Node ID */
    int32 NextNodeId = 1;

    /** Zone Actor 映射 (NodeId -> Actor) */
    UPROPERTY()
    TMap<FString, AMAZoneActor*> ZoneActors;
    
    /** Goal Actor 映射 (NodeId -> Actor) */
    UPROPERTY()
    TMap<FString, AMAGoalActor*> GoalActors;

    //=========================================================================
    // 内部方法
    //=========================================================================
    
    /**
     * 加载临时场景图
     * 
     * @return 加载是否成功
     */
    bool LoadTempSceneGraph();
    
    /**
     * 保存临时场景图
     * 
     * @return 保存是否成功
     */
    bool SaveTempSceneGraph();
    
    /**
     * 生成下一个可用 ID
     * 
     * @return 新的 ID 字符串
     */
    FString GenerateNextId();
    
    /**
     * 获取源场景图文件路径
     * 
     * @return 源文件路径
     */
    FString GetSourceSceneGraphPath() const;
    
    /**
     * 设置 Actor 高亮状态
     * 
     * @param Actor 目标 Actor
     * @param bHighlight 是否高亮
     */
    void SetActorHighlight(AActor* Actor, bool bHighlight);
    
    /**
     * 清除 Actor 高亮
     */
    void ClearActorHighlight();
    
    /**
     * 清除 POI 高亮
     */
    void ClearPOIHighlight();
    
    /**
     * 检查 Node 是否为 point 类型
     * Requirements: 5.3, 5.5, 7.5, 12.5
     * 
     * @param NodeObject Node JSON 对象
     * @return 是否为 point 类型
     */
    bool IsPointTypeNode(const TSharedPtr<FJsonObject>& NodeObject) const;
    
    /**
     * 根据 ID 查找 Node
     * 
     * @param NodeId Node ID
     * @return Node JSON 对象，未找到返回 nullptr
     */
    TSharedPtr<FJsonObject> FindNodeById(const FString& NodeId) const;
    
    /**
     * 根据 ID 查找 Node 索引
     * 
     * @param NodeId Node ID
     * @return Node 在数组中的索引，未找到返回 -1
     */
    int32 FindNodeIndexById(const FString& NodeId) const;
    
    /**
     * 删除与指定 Node 相关的所有 Edge
     * Requirements: 7.3
     * 
     * @param NodeId Node ID
     * @return 删除的 Edge 数量
     */
    int32 DeleteEdgesForNode(const FString& NodeId);
    
    /**
     * 同步 Actor 位置到场景
     * Requirements: 6.5, 12.2, 12.3
     * 
     * @param NodeObject Node JSON 对象
     */
    void SyncActorPositionFromNode(const TSharedPtr<FJsonObject>& NodeObject);
    
    /**
     * 计算凸包
     * Requirements: 10.3
     * 
     * @param Points 输入点集
     * @return 凸包顶点（逆时针顺序）
     */
    TArray<FVector> ComputeConvexHull(const TArray<FVector>& Points) const;
};
