// MAEditModeManager.h
// Edit Mode 管理器 - 负责 POI 管理、选择管理、Goal/Zone Actor 可视化管理
// 
// 重构后的架构:
// - 图操作 (AddNode/DeleteNode/EditNode) 委托给 MASceneGraphManager
// - 本类专注于 UI 交互和视觉反馈
//

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonObject.h"
#include "../Comm/MACommTypes.h"
#include "MAEditModeManager.generated.h"

class AMAPointOfInterest;
class AMAZoneActor;
class AMAGoalActor;
class UMASceneGraphManager;

// 日志类别
DECLARE_LOG_CATEGORY_EXTERN(LogMAEditMode, Log, All);

//=============================================================================
// 委托声明
//=============================================================================

/** 选择变化委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEditModeSelectionChanged);

/**
 * Edit Mode 管理器
 * 
 * 职责:
 * - POI (Point of Interest) 的创建和管理
 * - 对象选择管理 (Actor 单选、POI 多选、互斥选择)
 * - Goal/Zone Actor 可视化管理
 * - Goal/Zone 创建 (委托给 MASceneGraphManager)
 * 
 * 注意: 图操作 (AddNode/DeleteNode/EditNode) 已移至 MASceneGraphManager
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
    // POI 管理
    //=========================================================================
    
    /**
     * 在指定位置创建 POI
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
    //=========================================================================
    
    /**
     * 选择 Actor (单选，自动取消之前选择)
     * 
     * @param Actor 要选择的 Actor
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Selection")
    void SelectActor(AActor* Actor);
    
    /**
     * 选择 POI (多选)
     * 
     * @param POI 要选择的 POI
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Selection")
    void SelectPOI(AMAPointOfInterest* POI);
    
    /**
     * 从选择集合移除对象
     * 
     * @param Object 要移除的对象
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Selection")
    void DeselectObject(UObject* Object);
    
    /**
     * 清除所有选择
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
    // Edit Mode 状态
    //=========================================================================

    /**
     * 检查 Edit Mode 是否可用
     * Edit Mode 需要源场景图文件存在才能使用
     * 
     * @return Edit Mode 是否可用
     */
    UFUNCTION(BlueprintPure, Category = "EditMode")
    bool IsEditModeAvailable() const;

    /**
     * 根据 GUID 查找 Actor
     * 
     * @param Guid Actor 的 GUID 字符串
     * @return 找到的 Actor，未找到返回 nullptr
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode")
    AActor* FindActorByGuid(const FString& Guid) const;


    //=========================================================================
    // Goal/Zone 创建 (委托给 MASceneGraphManager)
    //=========================================================================
    
    /**
     * 创建 Goal Node
     * 内部调用 MASceneGraphManager::AddNode()
     * 
     * @param Location 位置
     * @param Description 描述
     * @param OutError 错误信息
     * @return 创建是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Goal")
    bool CreateGoal(const FVector& Location, const FString& Description, FString& OutError);
    
    /**
     * 创建 Zone Node
     * 内部调用 MASceneGraphManager::AddNode()
     * 
     * @param Vertices 顶点数组
     * @param Description 描述
     * @param OutError 错误信息
     * @return 创建是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "EditMode|Zone")
    bool CreateZone(const TArray<FVector>& Vertices, const FString& Description, FString& OutError);

    //=========================================================================
    // Zone/Goal Actor 管理
    //=========================================================================
    
    /**
     * 为 Zone Node 创建可视化 Actor
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
    // 委托
    //=========================================================================
    
    /** 选择变化委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEditModeSelectionChanged OnSelectionChanged;

private:
    //=========================================================================
    // 内部状态
    //=========================================================================
    
    /** POI 列表 */
    UPROPERTY()
    TArray<AMAPointOfInterest*> POIs;
    
    /** 选中的 Actor (单选) */
    UPROPERTY()
    AActor* SelectedActor = nullptr;
    
    /** 选中的 POI 列表 (多选) */
    UPROPERTY()
    TArray<AMAPointOfInterest*> SelectedPOIs;

    /** Zone Actor 映射 (NodeId -> Actor) */
    UPROPERTY()
    TMap<FString, AMAZoneActor*> ZoneActors;
    
    /** Goal Actor 映射 (NodeId -> Actor) */
    UPROPERTY()
    TMap<FString, AMAGoalActor*> GoalActors;
    
    /** 下一个可用的 Node ID (用于生成 Goal/Zone ID) */
    int32 NextNodeId = 1;

    //=========================================================================
    // 内部方法
    //=========================================================================
    
    /**
     * 获取 MASceneGraphManager 实例
     * 
     * @return MASceneGraphManager 指针，失败返回 nullptr
     */
    UMASceneGraphManager* GetSceneGraphManager() const;
    
    /**
     * 生成下一个可用 ID
     * 
     * @return 新的 ID 字符串
     */
    FString GenerateNextId();
    
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
     * 计算凸包
     * 
     * @param Points 输入点集
     * @return 凸包顶点（逆时针顺序）
     */
    TArray<FVector> ComputeConvexHull(const TArray<FVector>& Points) const;
};
