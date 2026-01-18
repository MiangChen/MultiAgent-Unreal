// MATaskPlannerWidget.h
// 任务规划工作台主容器 Widget - 纯 C++ 实现

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Core/Types/MATaskGraphTypes.h"
#include "MATaskPlannerWidget.generated.h"

class UMADAGCanvasWidget;
class UMANodePaletteWidget;
class UMATaskGraphModel;
class UMultiLineEditableTextBox;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UScrollBox;
class USplitter;

//=============================================================================
// 委托声明
//=============================================================================

/** 任务图变更委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTaskGraphChanged, const FMATaskGraphData&, NewData);

/** 指令提交委托 (兼容旧接口) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlannerCommandSubmitted, const FString&, Command);

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMATaskPlanner, Log, All);

//=============================================================================
// UMATaskPlannerWidget - 任务规划工作台主容器
//=============================================================================

/**
 * 任务规划工作台主容器 Widget
 * 
 * 布局结构:
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │                    Task Planner Workbench                           │
 * ├─────────────────────┬───────────────────────────────────────────────┤
 * │   Left Panel        │         Right Panel                           │
 * │  ┌───────────────┐  │  ┌─────────────────────────────────────────┐ │
 * │  │ Status Log    │  │  │     DAG Canvas                          │ │
 * │  │ (ReadOnly)    │  │  │  ┌─────────┐  ┌─────────┐              │ │
 * │  └───────────────┘  │  │  │ Node A  │──│ Node B  │              │ │
 * │  ┌───────────────┐  │  │  └─────────┘  └─────────┘              │ │
 * │  │ JSON Editor   │  │  │       │                                 │ │
 * │  │ (Editable)    │  │  │       ▼                                 │ │
 * │  └───────────────┘  │  │  ┌─────────┐                            │ │
 * │  ┌───────────────┐  │  │  │ Node C  │                            │ │
 * │  │ Update Button │  │  │  └─────────┘                            │ │
 * │  └───────────────┘  │  └─────────────────────────────────────────┘ │
 * │                     │  ┌─────────────────────────────────────────┐ │
 * │                     │  │     Node Palette (Sidebar)              │ │
 * │                     │  │  [Navigate] [Patrol] [Custom]           │ │
 * │                     │  └─────────────────────────────────────────┘ │
 * └─────────────────────┴───────────────────────────────────────────────┘
 * 
 */
UCLASS()
class MULTIAGENT_API UMATaskPlannerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMATaskPlannerWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口
    //=========================================================================

    /** 加载任务图数据 */
    UFUNCTION(BlueprintCallable, Category = "TaskPlanner")
    void LoadTaskGraph(const FMATaskGraphData& Data);

    /** 从 JSON 字符串加载任务图 */
    UFUNCTION(BlueprintCallable, Category = "TaskPlanner")
    bool LoadTaskGraphFromJson(const FString& JsonString);

    /** 追加状态日志消息 (带时间戳) */
    UFUNCTION(BlueprintCallable, Category = "TaskPlanner")
    void AppendStatusLog(const FString& Message);

    /** 清空状态日志 */
    UFUNCTION(BlueprintCallable, Category = "TaskPlanner")
    void ClearStatusLog();

    /** 获取 JSON 编辑器文本 */
    UFUNCTION(BlueprintPure, Category = "TaskPlanner")
    FString GetJsonText() const;

    /** 设置 JSON 编辑器文本 */
    UFUNCTION(BlueprintCallable, Category = "TaskPlanner")
    void SetJsonText(const FString& JsonText);

    /** 获取数据模型 */
    UFUNCTION(BlueprintPure, Category = "TaskPlanner")
    UMATaskGraphModel* GetGraphModel() const { return GraphModel; }

    /** 获取 DAG 画布 */
    UFUNCTION(BlueprintPure, Category = "TaskPlanner")
    UMADAGCanvasWidget* GetDAGCanvas() const { return DAGCanvas; }

    /** 将焦点设置到 JSON 编辑器 */
    UFUNCTION(BlueprintCallable, Category = "TaskPlanner")
    void FocusJsonEditor();

    /** 加载 Mock 数据 (从 datasets/response_example.json) */
    UFUNCTION(BlueprintCallable, Category = "TaskPlanner")
    bool LoadMockData();

    /** 检查是否为 Mock 模式 */
    UFUNCTION(BlueprintPure, Category = "TaskPlanner")
    bool IsMockMode() const { return bUseMockData; }

    //=========================================================================
    // 委托
    //=========================================================================

    /** 任务图变更委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskPlanner|Events")
    FOnTaskGraphChanged OnTaskGraphChanged;

    /** 指令提交委托 (兼容旧接口) */
    UPROPERTY(BlueprintAssignable, Category = "TaskPlanner|Events")
    FOnPlannerCommandSubmitted OnCommandSubmitted;

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    //=========================================================================
    // UI 构建
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 创建左侧面板 */
    UBorder* CreateLeftPanel();

    /** 创建右侧面板 */
    UBorder* CreateRightPanel();

    /** 创建状态日志区域 */
    UVerticalBox* CreateStatusLogSection();

    /** 创建 JSON 编辑器区域 */
    UVerticalBox* CreateJsonEditorSection();

    /** 创建用户指令输入区域 */
    UVerticalBox* CreateUserInputSection();

    //=========================================================================
    // 事件处理
    //=========================================================================

    /** "更新任务图" 按钮点击回调 */
    UFUNCTION()
    void OnUpdateGraphButtonClicked();

    /** "发送指令" 按钮点击回调 */
    UFUNCTION()
    void OnSendCommandButtonClicked();

    /** "提交任务图" 按钮点击回调 */
    UFUNCTION()
    void OnSubmitTaskGraphButtonClicked();

    /** 数据模型变更回调 */
    UFUNCTION()
    void OnModelDataChanged();

    /** 节点模板选中回调 */
    UFUNCTION()
    void OnNodeTemplateSelected(const FMANodeTemplate& Template);

    /** 节点删除请求回调 */
    UFUNCTION()
    void OnNodeDeleteRequested(const FString& NodeId);

    /** 节点编辑请求回调 (双击节点) */
    UFUNCTION()
    void OnNodeEditRequested(const FString& NodeId);

    /** 边创建回调 */
    UFUNCTION()
    void OnEdgeCreated(const FString& FromNodeId, const FString& ToNodeId);

    /** 边删除请求回调 */
    UFUNCTION()
    void OnEdgeDeleteRequested(const FString& FromNodeId, const FString& ToNodeId);

    /** 关闭按钮点击回调 */
    UFUNCTION()
    void OnCloseButtonClicked();

    //=========================================================================
    // 辅助方法
    //=========================================================================

    /** 同步 JSON 编辑器内容 (从模型) */
    void SyncJsonEditorFromModel();

    /** 获取当前时间戳字符串 */
    FString GetTimestamp() const;

    /** 从临时文件加载任务图数据 */
    void LoadFromTempFile();

    /** 绑定 TempDataManager 数据变更事件 */
    void BindTempDataManagerEvents();

    /** TempDataManager 任务图数据变更回调 */
    UFUNCTION()
    void OnTempDataTaskGraphChanged(const FMATaskGraphData& NewData);

    /** 保存任务图到临时文件 */
    void SaveToTempFile();

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 状态日志文本框 (只读) */
    UPROPERTY()
    UMultiLineEditableTextBox* StatusLogBox;

    /** JSON 编辑器文本框 (可编辑) */
    UPROPERTY()
    UMultiLineEditableTextBox* JsonEditorBox;

    /** "更新任务图" 按钮 */
    UPROPERTY()
    UButton* UpdateGraphButton;

    /** 用户指令输入框 */
    UPROPERTY()
    UMultiLineEditableTextBox* UserInputBox;

    /** "发送指令" 按钮 */
    UPROPERTY()
    UButton* SendCommandButton;

    /** "提交任务图" 按钮 */
    UPROPERTY()
    UButton* SubmitTaskGraphButton;

    /** 关闭按钮 (右上角 X) */
    UPROPERTY()
    UButton* CloseButton;

    /** DAG 画布 Widget */
    UPROPERTY()
    UMADAGCanvasWidget* DAGCanvas;

    /** 节点工具栏 Widget */
    UPROPERTY()
    UMANodePaletteWidget* NodePalette;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 任务图数据模型 */
    UPROPERTY()
    UMATaskGraphModel* GraphModel;

    //=========================================================================
    // 配置
    //=========================================================================

    /** 左侧面板宽度比例 */
    float LeftPanelWidthRatio = 0.35f;

    /** 状态日志高度比例 */
    float StatusLogHeightRatio = 0.3f;

    //=========================================================================
    // 颜色配置
    //=========================================================================

    /** 背景颜色 */
    FLinearColor BackgroundColor = FLinearColor(0.05f, 0.05f, 0.08f, 0.98f);

    /** 面板背景颜色 */
    FLinearColor PanelBackgroundColor = FLinearColor(0.08f, 0.08f, 0.1f, 1.0f);

    /** 标题颜色 */
    FLinearColor TitleColor = FLinearColor(0.9f, 0.9f, 1.0f, 1.0f);

    /** 标签颜色 */
    FLinearColor LabelColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /** 按钮颜色 */
    FLinearColor ButtonColor = FLinearColor(0.2f, 0.5f, 0.8f, 1.0f);

    /** 是否使用 Mock 数据 */
    bool bUseMockData = true;
};
