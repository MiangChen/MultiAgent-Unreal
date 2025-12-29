// MAEditModeManager.h
// Edit Mode 管理器 - 负责临时场景图的生命周期和所有编辑操作
// 
// Edit Mode 用于模拟任务执行过程中发生的"新情况"（动态变化）
// 与 Modify Mode（持久化修改源场景图文件）不同，Edit Mode 的所有操作仅针对临时场景图文件进行
//
// Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 3.1, 3.2, 3.4, 3.5, 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonObject.h"
#include "MAEditModeManager.generated.h"

class AMAPointOfInterest;

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
    // 后端通信 (将在后续任务中实现)
    //=========================================================================
    
    /**
     * 发送场景变化消息
     * 
     * @param ChangeType 变化类型
     * @param Payload 负载数据
     */
    void SendSceneChangeMessage(const FString& ChangeType, const FString& Payload);

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
};
