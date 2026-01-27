// MATaskNodeWidget.h
// 任务节点 Widget - DAG 画布中的单个任务节点可视化组件

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Core/Types/MATaskGraphTypes.h"
#include "MATaskNodeWidget.generated.h"

class UBorder;
class UTextBlock;
class UImage;
class UVerticalBox;
class UCanvasPanel;
class UCanvasPanelSlot;
class USizeBox;
class UMAUITheme;

//=============================================================================
// 委托声明
//=============================================================================

/** 节点选中委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTaskNodeSelected, const FString&, NodeId);

/** 节点拖拽开始委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTaskNodeDragStarted, const FString&, NodeId, FVector2D, StartPosition);

/** 节点拖拽中委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTaskNodeDragging, const FString&, NodeId, FVector2D, CurrentPosition);

/** 节点拖拽结束委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTaskNodeDragEnded, const FString&, NodeId, FVector2D, EndPosition);

/** 端口拖拽开始委托 (用于创建连线) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPortDragStarted, const FString&, NodeId, bool, bIsOutputPort, FVector2D, PortPosition);

/** 端口拖拽释放委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPortDragReleased, const FString&, NodeId, bool, bIsInputPort, FVector2D, PortPosition);

/** 节点双击委托 (用于编辑) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTaskNodeDoubleClicked, const FString&, NodeId);

/** 节点右键委托 (用于上下文菜单) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTaskNodeRightClicked, const FString&, NodeId, FVector2D, ScreenPosition);

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMATaskNodeWidget, Log, All);


//=============================================================================
// UMATaskNodeWidget - 任务节点 Widget
//=============================================================================

/**
 * 任务节点 Widget
 * 
 * DAG 画布中的单个任务节点可视化组件
 * 显示任务 ID、描述、位置信息，并提供输入/输出端口用于连线
 * 
 * 布局结构:
 * ┌─────────────────────────────────┐
 * │          ● (输入端口)           │
 * ├─────────────────────────────────┤
 * │  [Task ID]                      │
 * │  Description text...            │
 * │  📍 Location                    │
 * ├─────────────────────────────────┤
 * │          ● (输出端口)           │
 * └─────────────────────────────────┘
 */
UCLASS()
class MULTIAGENT_API UMATaskNodeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMATaskNodeWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 初始化
    //=========================================================================

    /** 使用节点数据初始化 */
    UFUNCTION(BlueprintCallable, Category = "TaskNode")
    void InitializeNode(const FMATaskNodeData& Data);

    //=========================================================================
    // 属性访问
    //=========================================================================

    /** 获取节点 ID */
    UFUNCTION(BlueprintPure, Category = "TaskNode")
    FString GetNodeId() const { return NodeData.TaskId; }

    /** 获取节点数据 */
    UFUNCTION(BlueprintPure, Category = "TaskNode")
    FMATaskNodeData GetNodeData() const { return NodeData; }

    /** 设置节点数据 */
    UFUNCTION(BlueprintCallable, Category = "TaskNode")
    void SetNodeData(const FMATaskNodeData& Data);

    /** 更新节点显示 */
    UFUNCTION(BlueprintCallable, Category = "TaskNode")
    void UpdateDisplay();

    /** 更新输出端口位置 (根据节点实际高度) */
    void UpdateOutputPortPosition();

    //=========================================================================
    // 端口位置
    //=========================================================================

    /** 获取输入端口位置 (相对于节点左上角) */
    UFUNCTION(BlueprintPure, Category = "TaskNode")
    FVector2D GetInputPortLocalPosition() const;

    /** 获取输出端口位置 (相对于节点左上角) */
    UFUNCTION(BlueprintPure, Category = "TaskNode")
    FVector2D GetOutputPortLocalPosition() const;

    /** 获取节点实际宽度 (考虑内容自适应) */
    float GetActualNodeWidth() const;

    /** 获取节点实际高度 (考虑内容自适应) */
    float GetActualNodeHeight() const;

    /** 获取输入端口世界位置 (相对于画布) */
    UFUNCTION(BlueprintPure, Category = "TaskNode")
    FVector2D GetInputPortPosition() const;

    /** 获取输出端口世界位置 (相对于画布) */
    UFUNCTION(BlueprintPure, Category = "TaskNode")
    FVector2D GetOutputPortPosition() const;

    /** 获取节点尺寸 */
    UFUNCTION(BlueprintPure, Category = "TaskNode")
    FVector2D GetNodeSize() const { return NodeSize; }

    //=========================================================================
    // 交互状态
    //=========================================================================

    /** 设置选中状态 */
    UFUNCTION(BlueprintCallable, Category = "TaskNode")
    void SetSelected(bool bInSelected);

    /** 获取选中状态 */
    UFUNCTION(BlueprintPure, Category = "TaskNode")
    bool IsSelected() const { return bIsSelected; }

    /** 设置高亮状态 */
    UFUNCTION(BlueprintCallable, Category = "TaskNode")
    void SetHighlighted(bool bInHighlighted);

    /** 获取高亮状态 */
    UFUNCTION(BlueprintPure, Category = "TaskNode")
    bool IsHighlighted() const { return bIsHighlighted; }

    /** 应用主题样式 */
    UFUNCTION(BlueprintCallable, Category = "TaskNode")
    void ApplyTheme(UMAUITheme* InTheme);

    /** 设置输入端口高亮 */
    UFUNCTION(BlueprintCallable, Category = "TaskNode")
    void SetInputPortHighlighted(bool bHighlighted);

    /** 设置输出端口高亮 */
    UFUNCTION(BlueprintCallable, Category = "TaskNode")
    void SetOutputPortHighlighted(bool bHighlighted);

    //=========================================================================
    // 委托
    //=========================================================================

    /** 节点选中委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskNode|Events")
    FOnTaskNodeSelected OnNodeSelected;

    /** 节点拖拽开始委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskNode|Events")
    FOnTaskNodeDragStarted OnDragStarted;

    /** 节点拖拽中委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskNode|Events")
    FOnTaskNodeDragging OnDragging;

    /** 节点拖拽结束委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskNode|Events")
    FOnTaskNodeDragEnded OnDragEnded;

    /** 端口拖拽开始委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskNode|Events")
    FOnPortDragStarted OnPortDragStarted;

    /** 端口拖拽释放委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskNode|Events")
    FOnPortDragReleased OnPortDragReleased;

    /** 节点双击委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskNode|Events")
    FOnTaskNodeDoubleClicked OnDoubleClicked;

    /** 节点右键委托 */
    UPROPERTY(BlueprintAssignable, Category = "TaskNode|Events")
    FOnTaskNodeRightClicked OnRightClicked;

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

    //=========================================================================
    // UI 构建
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 更新边框颜色 (根据选中/高亮状态) */
    void UpdateBorderColor();

    /** 更新端口颜色 */
    void UpdatePortColors();

    /** 生成 Tooltip 文本 */
    FString GenerateTooltipText() const;

    //=========================================================================
    // 端口命中检测
    //=========================================================================

    /** 检测点击是否在输入端口上 */
    bool IsPointOnInputPort(const FVector2D& LocalPosition) const;

    /** 检测点击是否在输出端口上 */
    bool IsPointOnOutputPort(const FVector2D& LocalPosition) const;

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 节点背景边框 */
    UPROPERTY()
    UBorder* NodeBackground;

    /** 主内容容器 */
    UPROPERTY()
    UVerticalBox* ContentBox;

    /** 任务 ID 文本 */
    UPROPERTY()
    UTextBlock* TaskIdText;

    /** 描述文本 */
    UPROPERTY()
    UTextBlock* DescriptionText;

    /** 位置文本 */
    UPROPERTY()
    UTextBlock* LocationText;

    /** 输入端口图像 */
    UPROPERTY()
    UBorder* InputPort;

    /** 输出端口图像 */
    UPROPERTY()
    UBorder* OutputPort;

    /** 输出端口的 Canvas Slot (用于动态更新位置) */
    UPROPERTY()
    UCanvasPanelSlot* OutputPortSlot;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 节点数据 */
    UPROPERTY()
    FMATaskNodeData NodeData;

    //=========================================================================
    // 状态
    //=========================================================================

    /** 是否选中 */
    bool bIsSelected = false;

    /** 是否高亮 (鼠标悬浮) */
    bool bIsHighlighted = false;

    /** 输入端口是否高亮 */
    bool bInputPortHighlighted = false;

    /** 输出端口是否高亮 */
    bool bOutputPortHighlighted = false;

    /** 是否正在拖拽节点 */
    bool bIsDraggingNode = false;

    /** 是否正在拖拽端口 */
    bool bIsDraggingPort = false;

    /** 拖拽的是输出端口 */
    bool bDraggingOutputPort = false;

    /** 拖拽起始屏幕位置 (用于节点拖拽时的屏幕坐标计算) */
    FVector2D DragStartScreenPosition;

    /** 拖拽开始时的画布位置 (用于计算正确的拖拽偏移) */
    FVector2D DragStartCanvasPosition;

    /** 端口拖拽起始位置 */
    FVector2D DragStartPosition;

    //=========================================================================
    // 配置
    //=========================================================================

    /** 节点尺寸 */
    FVector2D NodeSize = FVector2D(200.0f, 100.0f);

    /** 端口半径 */
    float PortRadius = 8.0f;

    /** 端口命中检测半径 (比视觉半径大，便于点击) */
    float PortHitRadius = 12.0f;

    //=========================================================================
    // 主题引用
    //=========================================================================

    /** 缓存的主题引用 */
    UPROPERTY()
    UMAUITheme* Theme;

    //=========================================================================
    // 颜色配置 (从 Theme 获取，带 fallback 默认值)
    //=========================================================================

    /** 默认背景颜色 */
    FLinearColor DefaultBackgroundColor = FLinearColor(0.15f, 0.15f, 0.2f, 0.95f);

    /** 选中背景颜色 */
    FLinearColor SelectedBackgroundColor = FLinearColor(0.2f, 0.3f, 0.5f, 0.95f);

    /** 高亮背景颜色 */
    FLinearColor HighlightedBackgroundColor = FLinearColor(0.2f, 0.2f, 0.3f, 0.95f);

    /** 默认边框颜色 */
    FLinearColor DefaultBorderColor = FLinearColor(0.3f, 0.3f, 0.4f, 1.0f);

    /** 选中边框颜色 */
    FLinearColor SelectedBorderColor = FLinearColor(0.4f, 0.6f, 1.0f, 1.0f);

    /** 高亮边框颜色 */
    FLinearColor HighlightedBorderColor = FLinearColor(0.5f, 0.5f, 0.6f, 1.0f);

    /** 默认端口颜色 */
    FLinearColor DefaultPortColor = FLinearColor(0.5f, 0.5f, 0.6f, 1.0f);

    /** 高亮端口颜色 */
    FLinearColor HighlightedPortColor = FLinearColor(0.3f, 0.8f, 0.3f, 1.0f);

    /** 标题文本颜色 */
    FLinearColor TitleTextColor = FLinearColor(0.9f, 0.9f, 1.0f, 1.0f);

    /** 描述文本颜色 */
    FLinearColor DescriptionTextColor = FLinearColor(0.7f, 0.7f, 0.8f, 1.0f);

    /** 位置文本颜色 */
    FLinearColor LocationTextColor = FLinearColor(0.5f, 0.7f, 0.5f, 1.0f);
};
