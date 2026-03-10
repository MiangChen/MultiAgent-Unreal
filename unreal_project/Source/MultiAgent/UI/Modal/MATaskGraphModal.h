// MATaskGraphModal.h
// 任务图模态窗口 - 用于查看和编辑任务 DAG
// 继承自 MABaseModalWidget，提供任务图详细可视化

#pragma once

#include "CoreMinimal.h"
#include "MABaseModalWidget.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"
#include "MATaskGraphModal.generated.h"

class UMADAGCanvasWidget;
class UMATaskGraphModel;
class UMultiLineEditableTextBox;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class UScrollBox;

//=============================================================================
// 委托声明
//=============================================================================

/** 任务图确认提交委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTaskGraphConfirmed, const FMATaskGraphData&, Data);

/** 任务图拒绝委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskGraphRejected);

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMATaskGraphModal, Log, All);

//=============================================================================
// UMATaskGraphModal - 任务图模态窗口
//=============================================================================

/**
 * 任务图模态窗口
 * 
 * 功能:
 * - 显示任务图 DAG 的详细可视化
 * - 支持只读模式（查看）和编辑模式
 * - 在编辑模式下可以修改任务图
 * - 提供 Confirm/Reject/Edit 按钮操作
 * 
 * 布局结构:
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │                        Task Graph                                    │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │  ┌─────────────────────────────────────────────────────────────┐   │
 * │  │                    DAG Canvas                                │   │
 * │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐                      │   │
 * │  │  │ Task A  │──│ Task B  │──│ Task C  │                      │   │
 * │  │  └─────────┘  └─────────┘  └─────────┘                      │   │
 * │  │       │                                                      │   │
 * │  │       ▼                                                      │   │
 * │  │  ┌─────────┐                                                 │   │
 * │  │  │ Task D  │                                                 │   │
 * │  │  └─────────┘                                                 │   │
 * │  └─────────────────────────────────────────────────────────────┘   │
 * │  ┌─────────────────────────────────────────────────────────────┐   │
 * │  │ JSON Preview (编辑模式下可编辑)                              │   │
 * │  └─────────────────────────────────────────────────────────────┘   │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │                    [Confirm] [Reject] [Edit]                        │
 * └─────────────────────────────────────────────────────────────────────┘
 * 
 */
UCLASS()
class MULTIAGENT_API UMATaskGraphModal : public UMABaseModalWidget
{
    GENERATED_BODY()

public:
    UMATaskGraphModal(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口
    //=========================================================================

    /**
     * 加载任务图数据
     * @param Data 任务图数据
     */
    UFUNCTION(BlueprintCallable, Category = "TaskGraphModal")
    void LoadTaskGraph(const FMATaskGraphData& Data);

    /**
     * 从 JSON 字符串加载任务图
     * @param JsonString JSON 字符串
     * @return 是否加载成功
     */
    UFUNCTION(BlueprintCallable, Category = "TaskGraphModal")
    bool LoadTaskGraphFromJson(const FString& JsonString);

    /**
     * 获取当前任务图数据
     * @return 当前任务图数据
     */
    UFUNCTION(BlueprintPure, Category = "TaskGraphModal")
    FMATaskGraphData GetTaskGraphData() const;

    /**
     * 获取数据模型
     * @return 任务图数据模型
     */
    UFUNCTION(BlueprintPure, Category = "TaskGraphModal")
    UMATaskGraphModel* GetGraphModel() const { return GraphModel; }

    /**
     * 获取 DAG 画布
     * @return DAG 画布 Widget
     */
    UFUNCTION(BlueprintPure, Category = "TaskGraphModal")
    UMADAGCanvasWidget* GetDAGCanvas() const { return DAGCanvas; }

    /**
     * 获取任务图 JSON 字符串
     * @return 任务图的 JSON 表示
     */
    UFUNCTION(BlueprintPure, Category = "TaskGraphModal")
    FString GetTaskGraphJson() const;

    //=========================================================================
    // 委托
    //=========================================================================

    /** 任务图确认提交委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskGraphModal|Events")
    FOnTaskGraphConfirmed OnTaskGraphConfirmed;

    /** 任务图拒绝委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskGraphModal|Events")
    FOnTaskGraphRejected OnTaskGraphRejected;

protected:
    //=========================================================================
    // UMABaseModalWidget 重写
    //=========================================================================

    /** 编辑模式变化回调 */
    virtual void OnEditModeChanged(bool bEditable) override;

    /** 获取模态标题 */
    virtual FText GetModalTitleText() const override;

    /** 构建内容区域 */
    virtual void BuildContentArea(UVerticalBox* InContentContainer) override;

    /** 主题应用后回调 */
    virtual void OnThemeApplied() override;

    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 创建 DAG 画布区域 */
    UBorder* CreateDAGCanvasArea();

    /** 创建 JSON 预览区域 */
    UVerticalBox* CreateJsonPreviewArea();

    /** 同步 JSON 预览内容 */
    void SyncJsonPreview();

    /** 从 JSON 编辑器更新模型 */
    void UpdateModelFromJson();

    /** 处理确认按钮点击 */
    UFUNCTION()
    void HandleConfirm();

    /** 处理拒绝按钮点击 */
    UFUNCTION()
    void HandleReject();

    //=========================================================================
    // 事件处理
    //=========================================================================

    /** 数据模型变更回调 */
    UFUNCTION()
    void OnModelDataChanged();

    /** "更新" 按钮点击回调 (编辑模式) */
    UFUNCTION()
    void OnUpdateButtonClicked();

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** DAG 画布 Widget */
    UPROPERTY()
    UMADAGCanvasWidget* DAGCanvas;

    /** DAG 画布容器 */
    UPROPERTY()
    UBorder* DAGCanvasContainer;

    /** JSON 预览/编辑文本框 */
    UPROPERTY()
    UMultiLineEditableTextBox* JsonPreviewBox;

    /** JSON 预览容器 */
    UPROPERTY()
    UVerticalBox* JsonPreviewContainer;

    /** JSON 预览标签 */
    UPROPERTY()
    UTextBlock* JsonPreviewLabel;

    /** "更新" 按钮 (编辑模式) */
    UPROPERTY()
    UButton* UpdateButton;

    /** JSON 预览滚动容器 */
    UPROPERTY()
    UScrollBox* JsonScrollBox;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 任务图数据模型 */
    UPROPERTY()
    UMATaskGraphModel* GraphModel;

    /** 原始数据 (用于拒绝时恢复) */
    FMATaskGraphData OriginalData;

    //=========================================================================
    // 配置
    //=========================================================================

    /** DAG 画布高度比例 */
    float DAGCanvasHeightRatio = 0.7f;

    /** JSON 预览高度比例 */
    float JsonPreviewHeightRatio = 0.3f;
};
