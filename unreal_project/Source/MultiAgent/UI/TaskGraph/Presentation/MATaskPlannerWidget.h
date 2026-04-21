// MATaskPlannerWidget.h
// 任务规划工作台主容器 Widget - 纯 C++ 实现

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"
#include "MATaskPlannerWidget.generated.h"

struct FMATaskPlannerActionResult;
class FMATaskPlannerRuntimeAdapter;
class UMADAGCanvasWidget;
class UMANodePaletteWidget;
class UMATaskGraphModel;
class UMAStyledButton;
class UMultiLineEditableTextBox;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UScrollBox;
class USplitter;
class UMAUITheme;

//=============================================================================
// 委托声明
//=============================================================================

/** 任务图变更委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTaskGraphChanged, const FMATaskGraphData&, NewData);

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
 * │  │ JSON Editor   │  │  │     DAG Canvas                          │ │
 * │  │ (Editable,    │  │  │  ┌─────────┐  ┌─────────┐              │ │
 * │  │  fills left)  │  │  │  │ Node A  │──│ Node B  │              │ │
 * │  └───────────────┘  │  │  └─────────┘  └─────────┘              │ │
 * │  ┌───────────────┐  │  │       │                                 │ │
 * │  │ Update / Save │  │  │       ▼                                 │ │
 * │  └───────────────┘  │  │  ┌─────────┐                            │ │
 * │                     │  │  │ Node C  │                            │ │
 * │                     │  │  └─────────┘                            │ │
 * │                     │  └─────────────────────────────────────────┘ │
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

    /** 追加诊断日志消息 (输出到控制台，带时间戳) */
    UFUNCTION(BlueprintCallable, Category = "TaskPlanner")
    void AppendStatusLog(const FString& Message);

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

    /** 获取节点工具栏 */
    UFUNCTION(BlueprintPure, Category = "TaskPlanner")
    UMANodePaletteWidget* GetNodePalette() const { return NodePalette; }

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
    // 主题
    //=========================================================================

    /** 应用主题样式 */
    UFUNCTION(BlueprintCallable, Category = "TaskPlanner")
    void ApplyTheme(UMAUITheme* InTheme);

    //=========================================================================
    // 委托
    //=========================================================================

    /** 任务图变更委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskPlanner|Events")
    FOnTaskGraphChanged OnTaskGraphChanged;

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

    /** 创建 JSON 编辑器区域 */
    UVerticalBox* CreateJsonEditorSection();

    //=========================================================================
    // 事件处理
    //=========================================================================

    /** "更新任务图" 按钮点击回调 */
    UFUNCTION()
    void OnUpdateGraphButtonClicked();

    /** "提交任务图" 按钮点击回调 */
    UFUNCTION()
    void OnSubmitTaskGraphButtonClicked();

    /** "保存" 按钮点击回调 */
    UFUNCTION()
    void OnSaveButtonClicked();

    /** 保存数据并导航到 Modal */
    void SaveAndNavigateToModal();

    /** 应用 coordinator 返回的结果 */
    void ApplyActionResult(const FMATaskPlannerActionResult& Result);

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

    /** 从运行时存储加载任务图数据 */
    void LoadRuntimeTaskGraph();

    /** 绑定运行时任务图数据变更事件 */
    void BindRuntimeEvents();

    /** 运行时任务图数据变更回调 */
    UFUNCTION()
    void OnRuntimeTaskGraphChanged(const FMATaskGraphData& NewData);

    /** 保存任务图到运行时存储 */
    bool PersistTaskGraph(FString* OutError = nullptr);

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** JSON 编辑器文本框 (可编辑) */
    UPROPERTY()
    UMultiLineEditableTextBox* JsonEditorBox;

    /** "更新任务图" 按钮 */
    UPROPERTY()
    UMAStyledButton* UpdateGraphButton;

    /** "保存" 按钮 (黄色，保存并返回 Modal) */
    UPROPERTY()
    UMAStyledButton* SaveButton;

    /** "提交任务图" 按钮 (已废弃，保留以避免编译错误) */
    UPROPERTY()
    UButton* SubmitTaskGraphButton;

    /** 关闭按钮 (右上角 X) */
    UPROPERTY()
    UButton* CloseButton;

    //=========================================================================
    // 主题相关 TextBlock 引用 (用于 ApplyTheme 更新颜色)
    //=========================================================================

    /** 标题文字 */
    UPROPERTY()
    UTextBlock* TitleText;

    /** 提示文字 */
    UPROPERTY()
    UTextBlock* HintText;

    /** 关闭按钮文字 */
    UPROPERTY()
    UTextBlock* CloseText;

    /** JSON 编辑器标签 */
    UPROPERTY()
    UTextBlock* JsonEditorLabel;

    /** 背景遮罩 */
    UPROPERTY()
    UBorder* BackgroundOverlay;

    /** JSON 编辑器 Border (圆角背景) */
    UPROPERTY()
    UBorder* JsonEditorBorder;

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

    //=========================================================================
    // 主题引用
    //=========================================================================

    /** 缓存的主题引用 */
    UPROPERTY()
    UMAUITheme* Theme;

    //=========================================================================
    // 颜色配置 (从 Theme 获取，带 fallback 默认值)
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

    friend class FMATaskPlannerRuntimeAdapter;
};
