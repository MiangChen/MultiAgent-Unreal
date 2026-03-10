// MATempDataManager.h
// 临时数据管理器 - 统一管理任务图和技能列表的临时文件
//
// 职责:
// - 管理 unreal_project/data 目录下的临时 JSON 文件
// - 提供任务图和技能列表的读写接口
// - 广播数据变更事件通知 UI 组件刷新
// - 可选的文件监听功能

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/Shared/Types/MATaskGraphTypes.h"
#include "MATempDataManager.generated.h"

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMATempData, Log, All);

//=============================================================================
// 委托定义
//=============================================================================

/** 任务图数据变更委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTaskGraphDataChanged, const FMATaskGraphData&, NewData);

/** 技能分配数据变更委托 (TempDataManager 专用，避免与 MASkillAllocationModel 中的同名委托冲突) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTempSkillAllocationDataChanged, const FMASkillAllocationData&, NewData);

/** 技能状态更新委托 - 执行过程中实时更新 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillStatusUpdated, int32, TimeStep, const FString&, RobotId, ESkillExecutionStatus, NewStatus);

//=============================================================================
// UMATempDataManager - 临时数据管理器
//=============================================================================

/**
 * 临时数据管理器
 * 
 * 作为 GameInstanceSubsystem 实现，提供任务图和技能分配的统一数据管理。
 * 数据存储在 unreal_project/data 目录下的 JSON 文件中。
 * 
 * 文件路径:
 * - task_graph_temp.json: 任务图数据
 * - skill_allocation_temp.json: 技能分配数据
 * 
 * 使用方式:
 *   UMATempDataManager* TempDataMgr = GetGameInstance()->GetSubsystem<UMATempDataManager>();
 *   if (TempDataMgr)
 *   {
 *       FMASkillAllocationData Data;
 *       if (TempDataMgr->LoadSkillAllocation(Data))
 *       {
 *           // 使用数据...
 *       }
 *   }
 */
UCLASS()
class MULTIAGENT_API UMATempDataManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 文件名常量
    //=========================================================================

    /** 任务图临时文件名 */
    static const FString TaskGraphFileName;

    /** 技能分配临时文件名 */
    static const FString SkillAllocationFileName;

    /** 数据目录名 */
    static const FString DataDirectoryName;

    //=========================================================================
    // 生命周期
    //=========================================================================

    /** 初始化子系统 */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** 清理子系统 */
    virtual void Deinitialize() override;

    //=========================================================================
    // 任务图操作
    //=========================================================================

    /**
     * 保存任务图数据到临时文件
     * @param Data 任务图数据
     * @return 是否保存成功
     */
    UFUNCTION(BlueprintCallable, Category = "TempData|TaskGraph")
    bool SaveTaskGraph(const FMATaskGraphData& Data);

    /**
     * 从临时文件加载任务图数据
     * @param OutData 输出的任务图数据
     * @return 是否加载成功
     */
    UFUNCTION(BlueprintCallable, Category = "TempData|TaskGraph")
    bool LoadTaskGraph(FMATaskGraphData& OutData);

    /**
     * 获取任务图 JSON 字符串
     * @return JSON 字符串，如果文件不存在或读取失败返回空字符串
     */
    UFUNCTION(BlueprintCallable, Category = "TempData|TaskGraph")
    FString GetTaskGraphJson() const;

    /**
     * 检查任务图临时文件是否存在
     * @return 文件是否存在
     */
    UFUNCTION(BlueprintPure, Category = "TempData|TaskGraph")
    bool TaskGraphFileExists() const;

    //=========================================================================
    // 技能分配操作
    //=========================================================================

    /**
     * 保存技能分配数据到临时文件
     * @param Data 技能分配数据
     * @return 是否保存成功
     */
    UFUNCTION(BlueprintCallable, Category = "TempData|SkillAllocation")
    bool SaveSkillAllocation(const FMASkillAllocationData& Data);

    /**
     * 从临时文件加载技能分配数据
     * @param OutData 输出的技能分配数据
     * @return 是否加载成功
     */
    UFUNCTION(BlueprintCallable, Category = "TempData|SkillAllocation")
    bool LoadSkillAllocation(FMASkillAllocationData& OutData);

    /**
     * 获取技能分配 JSON 字符串
     * @return JSON 字符串，如果文件不存在或读取失败返回空字符串
     */
    UFUNCTION(BlueprintCallable, Category = "TempData|SkillAllocation")
    FString GetSkillAllocationJson() const;

    /**
     * 检查技能分配临时文件是否存在
     * @return 文件是否存在
     */
    UFUNCTION(BlueprintPure, Category = "TempData|SkillAllocation")
    bool SkillAllocationFileExists() const;

    //=========================================================================
    // 文件路径
    //=========================================================================

    /**
     * 获取任务图临时文件完整路径
     * @return 文件完整路径
     */
    UFUNCTION(BlueprintPure, Category = "TempData")
    FString GetTaskGraphFilePath() const;

    /**
     * 获取技能分配临时文件完整路径
     * @return 文件完整路径
     */
    UFUNCTION(BlueprintPure, Category = "TempData")
    FString GetSkillAllocationFilePath() const;

    /**
     * 获取数据目录完整路径
     * @return 目录完整路径
     */
    UFUNCTION(BlueprintPure, Category = "TempData")
    FString GetDataDirectoryPath() const;

    //=========================================================================
    // 文件监听控制 (可选功能)
    //=========================================================================

    /**
     * 启动文件监听
     * 监听临时文件的变化，变化时自动重新加载并广播事件
     */
    UFUNCTION(BlueprintCallable, Category = "TempData")
    void StartFileWatching();

    /**
     * 停止文件监听
     */
    UFUNCTION(BlueprintCallable, Category = "TempData")
    void StopFileWatching();

    /**
     * 是否正在监听文件变化
     * @return 是否正在监听
     */
    UFUNCTION(BlueprintPure, Category = "TempData")
    bool IsFileWatching() const { return bIsFileWatching; }

    //=========================================================================
    // 技能状态实时更新
    //=========================================================================

    /**
     * 广播技能状态更新
     * 用于执行过程中实时更新 UI 显示
     * @param TimeStep 时间步
     * @param RobotId 机器人 ID
     * @param NewStatus 新状态
     */
    UFUNCTION(BlueprintCallable, Category = "TempData|SkillAllocation")
    void BroadcastSkillStatusUpdate(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus);

    //=========================================================================
    // 事件委托
    //=========================================================================

    /** 任务图数据变更事件 */
    UPROPERTY(BlueprintAssignable, Category = "TempData|Events")
    FOnTaskGraphDataChanged OnTaskGraphChanged;

    /** 技能分配数据变更事件 */
    UPROPERTY(BlueprintAssignable, Category = "TempData|Events")
    FOnTempSkillAllocationDataChanged OnSkillAllocationChanged;

    /** 技能状态实时更新事件 */
    UPROPERTY(BlueprintAssignable, Category = "TempData|Events")
    FOnSkillStatusUpdated OnSkillStatusUpdated;

private:
    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 确保数据目录存在
     * @return 目录是否存在或创建成功
     */
    bool EnsureDataDirectoryExists();

    /**
     * 写入 JSON 到文件
     * @param FilePath 文件路径
     * @param JsonContent JSON 内容
     * @return 是否写入成功
     */
    bool WriteJsonToFile(const FString& FilePath, const FString& JsonContent);

    /**
     * 从文件读取 JSON
     * @param FilePath 文件路径
     * @param OutJsonContent 输出的 JSON 内容
     * @return 是否读取成功
     */
    bool ReadJsonFromFile(const FString& FilePath, FString& OutJsonContent) const;

    /**
     * 文件变化回调
     * @param ChangedFiles 变化的文件列表
     */
    void OnFileChanged(const TArray<struct FFileChangeData>& ChangedFiles);

    /**
     * 处理文件变化 (带防抖)
     */
    void ProcessFileChange();

    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 数据目录路径 (缓存) */
    FString CachedDataDirectory;

    /** 任务图文件路径 (缓存) */
    FString CachedTaskGraphFilePath;

    /** 技能分配文件路径 (缓存) */
    FString CachedSkillAllocationFilePath;

    /** 是否正在监听文件变化 */
    bool bIsFileWatching = false;

    /** 文件监听句柄 */
    FDelegateHandle FileWatchHandle;

    /** 防抖定时器句柄 */
    FTimerHandle DebounceTimerHandle;

    /** 防抖延迟时间 (秒) */
    float DebounceDelay = 0.5f;

    /** 待处理的文件变化标志 */
    bool bTaskGraphFileChanged = false;
    bool bSkillAllocationFileChanged = false;

    /** 技能分配内存缓存 (用于保存状态更新) */
    FMASkillAllocationData CachedSkillAllocationData;
    
    /** 缓存是否有效 */
    bool bSkillAllocationCacheValid = false;
};
