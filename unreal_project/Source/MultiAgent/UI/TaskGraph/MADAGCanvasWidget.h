// MADAGCanvasWidget.h
// DAG 画布 Widget - 可视化任务图的画布组件

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Core/Types/MATaskGraphTypes.h"
#include "MADAGCanvasWidget.generated.h"

class UMATaskNodeWidget;
class UMATaskGraphModel;
class UMAContextMenuWidget;
class UCanvasPanel;
class UCanvasPanelSlot;
class UBorder;
class UMAUITheme;

//=============================================================================
// 边渲染数据
//=============================================================================

/** 边渲染数据 - 用于绘制连线 */
USTRUCT()
struct FMAEdgeRenderData
{
    GENERATED_BODY()

    /** 源节点 ID */
    FString FromNodeId;

    /** 目标节点 ID */
    FString ToNodeId;

    /** 边类型 */
    FString EdgeType;

    /** 起点位置 (画布坐标) */
    FVector2D StartPoint;

    /** 终点位置 (画布坐标) */
    FVector2D EndPoint;

    /** 是否高亮 */
    bool bIsHighlighted = false;

    FMAEdgeRenderData() {}

    FMAEdgeRenderData(const FString& InFrom, const FString& InTo, const FString& InType = TEXT("sequential"))
        : FromNodeId(InFrom), ToNodeId(InTo), EdgeType(InType) {}
};

//=============================================================================
// 委托声明
//=============================================================================

/** 节点选中委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCanvasNodeSelected, const FString&, NodeId);

/** 边创建委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCanvasEdgeCreated, const FString&, FromNodeId, const FString&, ToNodeId);

/** 边删除委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCanvasEdgeDeleted, const FString&, FromNodeId, const FString&, ToNodeId);

/** 节点删除请求委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCanvasNodeDeleteRequested, const FString&, NodeId);

/** 节点编辑请求委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCanvasNodeEditRequested, const FString&, NodeId);

/** 边删除请求委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCanvasEdgeDeleteRequested, const FString&, FromNodeId, const FString&, ToNodeId);

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMADAGCanvas, Log, All);

//=============================================================================
// UMADAGCanvasWidget - DAG 画布 Widget
//=============================================================================

/**
 * DAG 画布 Widget
 * 
 * 可视化任务图的画布组件，支持：
 * - 节点渲染和管理
 * - 边渲染 (箭头线)
 * - 平移和缩放
 * - 节点拖拽
 * - 连线拖拽创建
 * - 右键上下文菜单
 */
UCLASS()
class MULTIAGENT_API UMADAGCanvasWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMADAGCanvasWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 数据绑定
    //=========================================================================

    /** 绑定到数据模型 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void BindToModel(UMATaskGraphModel* Model);

    /** 从模型刷新画布 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void RefreshFromModel();

    /** 获取绑定的模型 */
    UFUNCTION(BlueprintPure, Category = "DAGCanvas")
    UMATaskGraphModel* GetModel() const { return GraphModel; }

    //=========================================================================
    // 节点操作
    //=========================================================================

    /** 创建节点 Widget */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    UMATaskNodeWidget* CreateNode(const FMATaskNodeData& NodeData, FVector2D Position);

    /** 移除节点 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void RemoveNode(const FString& NodeId);

    /** 移动节点 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void MoveNode(const FString& NodeId, FVector2D NewPosition);

    /** 获取节点 Widget */
    UFUNCTION(BlueprintPure, Category = "DAGCanvas")
    UMATaskNodeWidget* GetNodeWidget(const FString& NodeId) const;

    /** 清空所有节点 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void ClearAllNodes();

    //=========================================================================
    // 边操作
    //=========================================================================

    /** 创建边 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void CreateEdge(const FString& FromNodeId, const FString& ToNodeId, const FString& EdgeType = TEXT("sequential"));

    /** 移除边 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void RemoveEdge(const FString& FromNodeId, const FString& ToNodeId);

    /** 更新边端点位置 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void UpdateEdgePositions();

    /** 清空所有边 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void ClearAllEdges();

    //=========================================================================
    // 视图操作
    //=========================================================================

    /** 设置视图偏移 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void SetViewOffset(FVector2D Offset);

    /** 获取视图偏移 */
    UFUNCTION(BlueprintPure, Category = "DAGCanvas")
    FVector2D GetViewOffset() const { return ViewOffset; }

    /** 设置缩放级别 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void SetZoomLevel(float Zoom);

    /** 获取缩放级别 */
    UFUNCTION(BlueprintPure, Category = "DAGCanvas")
    float GetZoomLevel() const { return ZoomLevel; }

    /** 重置视图 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void ResetView();

    //=========================================================================
    // 选择
    //=========================================================================

    /** 选中节点 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void SelectNode(const FString& NodeId);

    /** 取消选中 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void ClearSelection();

    /** 选中边 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void SelectEdge(const FString& FromNodeId, const FString& ToNodeId);

    /** 取消边选中 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void ClearEdgeSelection();

    /** 获取选中的节点 ID */
    UFUNCTION(BlueprintPure, Category = "DAGCanvas")
    FString GetSelectedNodeId() const { return SelectedNodeId; }

    /** 删除当前选中的元素 (节点或边) */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void DeleteSelectedElement();

    //=========================================================================
    // 主题
    //=========================================================================

    /** 应用主题样式 */
    UFUNCTION(BlueprintCallable, Category = "DAGCanvas")
    void ApplyTheme(UMAUITheme* InTheme);

    //=========================================================================
    // 委托
    //=========================================================================

    /** 节点选中委托 */
    UPROPERTY(BlueprintAssignable, Category = "DAGCanvas|Events")
    FOnCanvasNodeSelected OnNodeSelected;

    /** 边创建委托 */
    UPROPERTY(BlueprintAssignable, Category = "DAGCanvas|Events")
    FOnCanvasEdgeCreated OnEdgeCreated;

    /** 边删除委托 */
    UPROPERTY(BlueprintAssignable, Category = "DAGCanvas|Events")
    FOnCanvasEdgeDeleted OnEdgeDeleted;

    /** 节点删除请求委托 */
    UPROPERTY(BlueprintAssignable, Category = "DAGCanvas|Events")
    FOnCanvasNodeDeleteRequested OnNodeDeleteRequested;

    /** 节点编辑请求委托 */
    UPROPERTY(BlueprintAssignable, Category = "DAGCanvas|Events")
    FOnCanvasNodeEditRequested OnNodeEditRequested;

    /** 边删除请求委托 */
    UPROPERTY(BlueprintAssignable, Category = "DAGCanvas|Events")
    FOnCanvasEdgeDeleteRequested OnEdgeDeleteRequested;

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
                              const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, 
                              int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual bool NativeSupportsKeyboardFocus() const override { return true; }

    //=========================================================================
    // UI 构建
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 更新节点位置 (应用视图变换) */
    void UpdateNodePositions();

    //=========================================================================
    // 绘制
    //=========================================================================

    /** 绘制所有边 */
    void DrawEdges(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制单条边 (带箭头) */
    void DrawEdge(const FMAEdgeRenderData& Edge, const FGeometry& AllottedGeometry, 
                  FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制预览连线 */
    void DrawPreviewEdge(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** 绘制箭头 */
    void DrawArrow(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color,
                   const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    //=========================================================================
    // 节点事件处理
    //=========================================================================

    /** 节点选中回调 */
    UFUNCTION()
    void OnNodeWidgetSelected(const FString& NodeId);

    /** 节点拖拽开始回调 */
    UFUNCTION()
    void OnNodeWidgetDragStarted(const FString& NodeId, FVector2D StartPosition);

    /** 节点拖拽中回调 */
    UFUNCTION()
    void OnNodeWidgetDragging(const FString& NodeId, FVector2D CurrentPosition);

    /** 节点拖拽结束回调 */
    UFUNCTION()
    void OnNodeWidgetDragEnded(const FString& NodeId, FVector2D EndPosition);

    /** 端口拖拽开始回调 */
    UFUNCTION()
    void OnNodePortDragStarted(const FString& NodeId, bool bIsOutputPort, FVector2D PortPosition);

    /** 端口拖拽释放回调 */
    UFUNCTION()
    void OnNodePortDragReleased(const FString& NodeId, bool bIsInputPort, FVector2D PortPosition);

    /** 节点双击回调 */
    UFUNCTION()
    void OnNodeWidgetDoubleClicked(const FString& NodeId);

    /** 节点右键回调 */
    UFUNCTION()
    void OnNodeWidgetRightClicked(const FString& NodeId, FVector2D ScreenPosition);

    //=========================================================================
    // 上下文菜单
    //=========================================================================

    /** 显示节点上下文菜单 */
    void ShowNodeContextMenu(const FString& NodeId, FVector2D ScreenPosition);

    /** 显示边上下文菜单 */
    void ShowEdgeContextMenu(const FString& FromNodeId, const FString& ToNodeId, FVector2D ScreenPosition);

    /** 上下文菜单项点击回调 */
    UFUNCTION()
    void OnContextMenuItemClicked(const FString& ItemId);

    /** 上下文菜单关闭回调 */
    UFUNCTION()
    void OnContextMenuClosed();

    //=========================================================================
    // 模型事件处理
    //=========================================================================

    /** 模型数据变更回调 */
    UFUNCTION()
    void OnModelDataChanged();

    //=========================================================================
    // 辅助方法
    //=========================================================================

    /** 画布坐标转换为屏幕坐标 */
    FVector2D CanvasToScreen(FVector2D CanvasPos) const;

    /** 屏幕坐标转换为画布坐标 */
    FVector2D ScreenToCanvas(FVector2D ScreenPos) const;

    /** 检测点击是否在边上 */
    bool IsPointOnEdge(const FVector2D& Point, const FMAEdgeRenderData& Edge, float Tolerance = 5.0f) const;

    /** 查找点击位置的边 */
    FMAEdgeRenderData* FindEdgeAtPoint(const FVector2D& Point);

    /** 查找点击位置的节点输入端口 */
    FString FindInputPortAtPoint(const FVector2D& ScreenPoint) const;

    /** 自动布局节点 */
    void AutoLayoutNodes();

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 画布背景 */
    UPROPERTY()
    UBorder* CanvasBackground;

    /** 节点容器 */
    UPROPERTY()
    UCanvasPanel* NodeContainer;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 绑定的数据模型 */
    UPROPERTY()
    UMATaskGraphModel* GraphModel;

    /** 节点 Widget 映射 */
    UPROPERTY()
    TMap<FString, UMATaskNodeWidget*> NodeWidgets;

    /** 边渲染数据 */
    TArray<FMAEdgeRenderData> EdgeRenderData;

    //=========================================================================
    // 视图状态
    //=========================================================================

    /** 视图偏移 */
    FVector2D ViewOffset = FVector2D::ZeroVector;

    /** 缩放级别 */
    float ZoomLevel = 1.0f;

    /** 最小缩放 */
    float MinZoom = 0.25f;

    /** 最大缩放 */
    float MaxZoom = 2.0f;

    //=========================================================================
    // 交互状态
    //=========================================================================

    /** 选中的节点 ID */
    FString SelectedNodeId;

    /** 选中的边 (From, To) */
    FString SelectedEdgeFrom;
    FString SelectedEdgeTo;

    /** 是否正在拖拽画布 */
    bool bIsDraggingCanvas = false;

    /** 画布拖拽起始位置 */
    FVector2D CanvasDragStart;

    /** 画布拖拽起始偏移 */
    FVector2D CanvasDragStartOffset;

    /** 是否正在拖拽连线 */
    bool bIsDraggingEdge = false;

    /** 连线拖拽源节点 ID */
    FString DragSourceNodeId;

    /** 连线拖拽起点 */
    FVector2D DragEdgeStart;

    /** 连线拖拽终点 */
    FVector2D DragEdgeEnd;

    /** 高亮的目标节点 ID (连线拖拽时) */
    FString HighlightedTargetNodeId;

    /** 上下文菜单 Widget */
    UPROPERTY()
    UMAContextMenuWidget* ContextMenu;

    /** 上下文菜单目标节点 ID */
    FString ContextMenuTargetNodeId;

    /** 上下文菜单目标边 (From) */
    FString ContextMenuTargetEdgeFrom;

    /** 上下文菜单目标边 (To) */
    FString ContextMenuTargetEdgeTo;

    //=========================================================================
    // 主题引用
    //=========================================================================

    /** 缓存的主题引用 */
    UPROPERTY()
    UMAUITheme* Theme;

    //=========================================================================
    // 颜色配置 (从 Theme 获取，带 fallback 默认值)
    //=========================================================================

    /** 画布背景颜色 */
    FLinearColor CanvasBackgroundColor = FLinearColor(0.08f, 0.08f, 0.1f, 1.0f);

    /** 默认边颜色 */
    FLinearColor DefaultEdgeColor = FLinearColor(0.5f, 0.5f, 0.6f, 1.0f);

    /** 高亮边颜色 */
    FLinearColor HighlightedEdgeColor = FLinearColor(0.3f, 0.8f, 0.3f, 1.0f);

    /** 预览边颜色 */
    FLinearColor PreviewEdgeColor = FLinearColor(0.8f, 0.8f, 0.3f, 0.8f);

    /** 边线宽度 */
    float EdgeThickness = 2.0f;

    /** 箭头大小 */
    float ArrowSize = 10.0f;
};
