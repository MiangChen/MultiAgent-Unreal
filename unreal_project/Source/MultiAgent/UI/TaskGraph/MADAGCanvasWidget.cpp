// MADAGCanvasWidget.cpp
// DAG 画布 Widget 实现

#include "MADAGCanvasWidget.h"
#include "MATaskNodeWidget.h"
#include "MATaskGraphModel.h"
#include "Application/MADAGCanvasAutoLayout.h"
#include "Infrastructure/MADAGCanvasGeometry.h"
#include "../Components/MAContextMenuWidget.h"
#include "../Core/MAUITheme.h"
#include "../Core/MARoundedBorderUtils.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"

DEFINE_LOG_CATEGORY(LogMADAGCanvas);

//=============================================================================
// 构造函数
//=============================================================================

UMADAGCanvasWidget::UMADAGCanvasWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

//=============================================================================
// 数据绑定
//=============================================================================

void UMADAGCanvasWidget::BindToModel(UMATaskGraphModel* Model)
{
    // 解绑旧模型
    if (GraphModel)
    {
        GraphModel->OnDataChanged.RemoveDynamic(this, &UMADAGCanvasWidget::OnModelDataChanged);
    }

    GraphModel = Model;

    // 绑定新模型
    if (GraphModel)
    {
        GraphModel->OnDataChanged.AddDynamic(this, &UMADAGCanvasWidget::OnModelDataChanged);
        RefreshFromModel();
    }

    UE_LOG(LogMADAGCanvas, Log, TEXT("Bound to model: %s"), GraphModel ? TEXT("Valid") : TEXT("Null"));
}

void UMADAGCanvasWidget::RefreshFromModel()
{
    if (!GraphModel)
    {
        UE_LOG(LogMADAGCanvas, Warning, TEXT("RefreshFromModel: No model bound"));
        return;
    }

    // 清空现有内容
    ClearAllNodes();
    ClearAllEdges();

    // 获取数据
    FMATaskGraphData Data = GraphModel->GetWorkingData();

    // 创建节点
    for (const FMATaskNodeData& NodeData : Data.Nodes)
    {
        FVector2D Position = NodeData.CanvasPosition;
        if (Position == FVector2D::ZeroVector)
        {
            // 如果没有位置，使用自动布局
            Position = FVector2D(100 + NodeWidgets.Num() * 250, 100);
        }
        CreateNode(NodeData, Position);
    }

    // 创建边
    for (const FMATaskEdgeData& EdgeData : Data.Edges)
    {
        CreateEdge(EdgeData.FromNodeId, EdgeData.ToNodeId, EdgeData.EdgeType);
    }

    // 如果节点没有位置，执行自动布局
    bool bNeedsLayout = false;
    for (const FMATaskNodeData& NodeData : Data.Nodes)
    {
        if (NodeData.CanvasPosition == FVector2D::ZeroVector)
        {
            bNeedsLayout = true;
            break;
        }
    }
    if (bNeedsLayout)
    {
        AutoLayoutNodes();
    }

    UpdateEdgePositions();

    UE_LOG(LogMADAGCanvas, Log, TEXT("Refreshed from model: %d nodes, %d edges"), 
           NodeWidgets.Num(), EdgeRenderData.Num());
}

//=============================================================================
// 节点操作
//=============================================================================

UMATaskNodeWidget* UMADAGCanvasWidget::CreateNode(const FMATaskNodeData& NodeData, FVector2D Position)
{
    if (!NodeContainer)
    {
        UE_LOG(LogMADAGCanvas, Error, TEXT("CreateNode: NodeContainer is null"));
        return nullptr;
    }

    // 检查是否已存在
    if (NodeWidgets.Contains(NodeData.TaskId))
    {
        UE_LOG(LogMADAGCanvas, Warning, TEXT("Node '%s' already exists"), *NodeData.TaskId);
        return NodeWidgets[NodeData.TaskId];
    }

    // 创建节点 Widget
    UMATaskNodeWidget* NodeWidget = CreateWidget<UMATaskNodeWidget>(this, UMATaskNodeWidget::StaticClass());
    if (!NodeWidget)
    {
        UE_LOG(LogMADAGCanvas, Error, TEXT("Failed to create node widget for '%s'"), *NodeData.TaskId);
        return nullptr;
    }

    // 初始化节点数据
    FMATaskNodeData MutableData = NodeData;
    MutableData.CanvasPosition = Position;
    NodeWidget->InitializeNode(MutableData);

    // 绑定事件
    NodeWidget->OnNodeSelected.AddDynamic(this, &UMADAGCanvasWidget::OnNodeWidgetSelected);
    NodeWidget->OnDragStarted.AddDynamic(this, &UMADAGCanvasWidget::OnNodeWidgetDragStarted);
    NodeWidget->OnDragging.AddDynamic(this, &UMADAGCanvasWidget::OnNodeWidgetDragging);
    NodeWidget->OnDragEnded.AddDynamic(this, &UMADAGCanvasWidget::OnNodeWidgetDragEnded);
    NodeWidget->OnPortDragStarted.AddDynamic(this, &UMADAGCanvasWidget::OnNodePortDragStarted);
    NodeWidget->OnPortDragReleased.AddDynamic(this, &UMADAGCanvasWidget::OnNodePortDragReleased);
    NodeWidget->OnDoubleClicked.AddDynamic(this, &UMADAGCanvasWidget::OnNodeWidgetDoubleClicked);
    NodeWidget->OnRightClicked.AddDynamic(this, &UMADAGCanvasWidget::OnNodeWidgetRightClicked);

    // 添加到容器
    UCanvasPanelSlot* Slot = NodeContainer->AddChildToCanvas(NodeWidget);
    if (Slot)
    {
        FVector2D ScreenPos = CanvasToScreen(Position);
        Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
        Slot->SetPosition(ScreenPos);
        Slot->SetAutoSize(true);
    }

    // 存储引用
    NodeWidgets.Add(NodeData.TaskId, NodeWidget);

    UE_LOG(LogMADAGCanvas, Verbose, TEXT("Created node: %s at (%.1f, %.1f)"), 
           *NodeData.TaskId, Position.X, Position.Y);

    return NodeWidget;
}

void UMADAGCanvasWidget::RemoveNode(const FString& NodeId)
{
    UMATaskNodeWidget** WidgetPtr = NodeWidgets.Find(NodeId);
    if (!WidgetPtr || !*WidgetPtr)
    {
        UE_LOG(LogMADAGCanvas, Warning, TEXT("RemoveNode: Node '%s' not found"), *NodeId);
        return;
    }

    // 移除关联的边
    EdgeRenderData.RemoveAll([&NodeId](const FMAEdgeRenderData& Edge) {
        return Edge.FromNodeId == NodeId || Edge.ToNodeId == NodeId;
    });

    // 从容器移除
    (*WidgetPtr)->RemoveFromParent();
    NodeWidgets.Remove(NodeId);

    // 清除选中状态
    if (CanvasState.SelectedNodeId == NodeId)
    {
        CanvasState.SelectedNodeId.Empty();
    }

    UE_LOG(LogMADAGCanvas, Verbose, TEXT("Removed node: %s"), *NodeId);
}

void UMADAGCanvasWidget::MoveNode(const FString& NodeId, FVector2D NewPosition)
{
    UMATaskNodeWidget** WidgetPtr = NodeWidgets.Find(NodeId);
    if (!WidgetPtr || !*WidgetPtr)
    {
        return;
    }

    UMATaskNodeWidget* Widget = *WidgetPtr;
    
    // 更新节点数据
    FMATaskNodeData Data = Widget->GetNodeData();
    Data.CanvasPosition = NewPosition;
    Widget->SetNodeData(Data);

    // 更新 Widget 位置
    UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot);
    if (Slot)
    {
        FVector2D ScreenPos = CanvasToScreen(NewPosition);
        Slot->SetPosition(ScreenPos);
    }

    // 更新模型中的位置
    if (GraphModel)
    {
        GraphModel->UpdateNodePosition(NodeId, NewPosition);
    }

    // 更新连线位置
    UpdateEdgePositions();
}

UMATaskNodeWidget* UMADAGCanvasWidget::GetNodeWidget(const FString& NodeId) const
{
    const UMATaskNodeWidget* const* WidgetPtr = NodeWidgets.Find(NodeId);
    return WidgetPtr ? const_cast<UMATaskNodeWidget*>(*WidgetPtr) : nullptr;
}

void UMADAGCanvasWidget::ClearAllNodes()
{
    for (auto& Pair : NodeWidgets)
    {
        if (Pair.Value)
        {
            Pair.Value->RemoveFromParent();
        }
    }
    NodeWidgets.Empty();
    CanvasState.SelectedNodeId.Empty();
}

//=============================================================================
// 边操作
//=============================================================================

void UMADAGCanvasWidget::CreateEdge(const FString& FromNodeId, const FString& ToNodeId, const FString& EdgeType)
{
    // 检查边是否已存在
    for (const FMAEdgeRenderData& Edge : EdgeRenderData)
    {
        if (Edge.FromNodeId == FromNodeId && Edge.ToNodeId == ToNodeId)
        {
            UE_LOG(LogMADAGCanvas, Warning, TEXT("Edge from '%s' to '%s' already exists"), *FromNodeId, *ToNodeId);
            return;
        }
    }

    // 创建边渲染数据
    FMAEdgeRenderData NewEdge(FromNodeId, ToNodeId, EdgeType);
    EdgeRenderData.Add(NewEdge);

    // 更新端点位置
    UpdateEdgePositions();

    UE_LOG(LogMADAGCanvas, Verbose, TEXT("Created edge: %s -> %s"), *FromNodeId, *ToNodeId);
}

void UMADAGCanvasWidget::RemoveEdge(const FString& FromNodeId, const FString& ToNodeId)
{
    int32 RemovedCount = EdgeRenderData.RemoveAll([&FromNodeId, &ToNodeId](const FMAEdgeRenderData& Edge) {
        return Edge.FromNodeId == FromNodeId && Edge.ToNodeId == ToNodeId;
    });

    if (RemovedCount > 0)
    {
        UE_LOG(LogMADAGCanvas, Verbose, TEXT("Removed edge: %s -> %s"), *FromNodeId, *ToNodeId);
    }
}

void UMADAGCanvasWidget::UpdateEdgePositions()
{
    FMADAGCanvasGeometry::UpdateEdgePositions(
        EdgeRenderData,
        NodeWidgets,
        CanvasState.ZoomLevel,
        CanvasState.ViewOffset);
}

void UMADAGCanvasWidget::ClearAllEdges()
{
    EdgeRenderData.Empty();
}

//=============================================================================
// 主题应用
//=============================================================================

void UMADAGCanvasWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme)
    {
        return;
    }
    
    // 从 Theme 更新颜色配置
    CanvasBackgroundColor = Theme->CanvasBackgroundColor;
    DefaultEdgeColor = Theme->EdgeColor;
    HighlightedEdgeColor = Theme->HighlightedEdgeColor;
    // PreviewEdgeColor 保持特殊值 (黄色预览线)
    
    // 更新画布背景颜色 - 使用圆角效果
    if (CanvasBackground)
    {
        MARoundedBorderUtils::ApplyRoundedCorners(CanvasBackground, CanvasBackgroundColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    }
    
    // 将主题传递给所有节点 Widget
    for (auto& Pair : NodeWidgets)
    {
        if (Pair.Value)
        {
            Pair.Value->ApplyTheme(Theme);
        }
    }
    
    UE_LOG(LogMADAGCanvas, Verbose, TEXT("ApplyTheme: Theme applied to DAG canvas"));
}


//=============================================================================
// 视图操作
//=============================================================================

void UMADAGCanvasWidget::SetViewOffset(FVector2D Offset)
{
    CanvasState.ViewOffset = Offset;
    UpdateNodePositions();
    UpdateEdgePositions();
}

void UMADAGCanvasWidget::SetZoomLevel(float Zoom)
{
    CanvasState.ZoomLevel = FMath::Clamp(Zoom, CanvasState.MinZoom, CanvasState.MaxZoom);
    UpdateNodePositions();
    UpdateEdgePositions();
}

void UMADAGCanvasWidget::ResetView()
{
    CanvasState.ViewOffset = FVector2D::ZeroVector;
    CanvasState.ZoomLevel = 1.0f;
    UpdateNodePositions();
    UpdateEdgePositions();
}

//=============================================================================
// 选择
//=============================================================================

void UMADAGCanvasWidget::SelectNode(const FString& NodeId)
{
    // 取消之前的选中
    if (!CanvasState.SelectedNodeId.IsEmpty() && CanvasState.SelectedNodeId != NodeId)
    {
        UMATaskNodeWidget* OldWidget = GetNodeWidget(CanvasState.SelectedNodeId);
        if (OldWidget)
        {
            OldWidget->SetSelected(false);
        }
    }

    CanvasState.SelectedNodeId = NodeId;

    // 选中新节点
    UMATaskNodeWidget* NewWidget = GetNodeWidget(NodeId);
    if (NewWidget)
    {
        NewWidget->SetSelected(true);
    }

    OnNodeSelected.Broadcast(NodeId);
}

void UMADAGCanvasWidget::ClearSelection()
{
    if (!CanvasState.SelectedNodeId.IsEmpty())
    {
        UMATaskNodeWidget* Widget = GetNodeWidget(CanvasState.SelectedNodeId);
        if (Widget)
        {
            Widget->SetSelected(false);
        }
        CanvasState.SelectedNodeId.Empty();
    }
    
    // 同时清除边选择
    ClearEdgeSelection();
}

void UMADAGCanvasWidget::SelectEdge(const FString& FromNodeId, const FString& ToNodeId)
{
    // 先清除之前的选择
    ClearEdgeSelection();
    ClearSelection();
    
    // 设置新的边选择
    CanvasState.SelectedEdgeFrom = FromNodeId;
    CanvasState.SelectedEdgeTo = ToNodeId;
    
    // 高亮选中的边
    for (FMAEdgeRenderData& Edge : EdgeRenderData)
    {
        if (Edge.FromNodeId == FromNodeId && Edge.ToNodeId == ToNodeId)
        {
            Edge.bIsHighlighted = true;
            break;
        }
    }
    
    UE_LOG(LogMADAGCanvas, Log, TEXT("Selected edge: %s -> %s"), *FromNodeId, *ToNodeId);
}

void UMADAGCanvasWidget::ClearEdgeSelection()
{
    if (!CanvasState.SelectedEdgeFrom.IsEmpty() || !CanvasState.SelectedEdgeTo.IsEmpty())
    {
        // 取消高亮
        for (FMAEdgeRenderData& Edge : EdgeRenderData)
        {
            Edge.bIsHighlighted = false;
        }
        
        CanvasState.SelectedEdgeFrom.Empty();
        CanvasState.SelectedEdgeTo.Empty();
    }
}

void UMADAGCanvasWidget::DeleteSelectedElement()
{
    // 优先删除选中的节点
    if (!CanvasState.SelectedNodeId.IsEmpty())
    {
        OnNodeDeleteRequested.Broadcast(CanvasState.SelectedNodeId);
        CanvasState.SelectedNodeId.Empty();
        return;
    }
    
    // 其次删除选中的边
    if (!CanvasState.SelectedEdgeFrom.IsEmpty() && !CanvasState.SelectedEdgeTo.IsEmpty())
    {
        OnEdgeDeleteRequested.Broadcast(CanvasState.SelectedEdgeFrom, CanvasState.SelectedEdgeTo);
        ClearEdgeSelection();
        return;
    }
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMADAGCanvasWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMADAGCanvasWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogMADAGCanvas, Verbose, TEXT("NativeConstruct"));
}

TSharedRef<SWidget> UMADAGCanvasWidget::RebuildWidget()
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }

    if (!WidgetTree->RootWidget)
    {
        BuildUI();
    }

    return Super::RebuildWidget();
}

int32 UMADAGCanvasWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                       const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                       int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    // 检查父 Widget 是否可见，如果不可见则不绘制
    if (!IsVisible() || GetVisibility() == ESlateVisibility::Collapsed || GetVisibility() == ESlateVisibility::Hidden)
    {
        return LayerId;
    }

    // 先绘制子 Widget
    int32 MaxLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, 
                                          LayerId, InWidgetStyle, bParentEnabled);

    // 推入裁剪区域，确保边线不会绘制到 Widget 边界之外
    OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));

    // 绘制边 (在节点下方)
    DrawEdges(AllottedGeometry, OutDrawElements, LayerId);

    // 绘制预览连线
    if (CanvasState.bIsDraggingEdge)
    {
        DrawPreviewEdge(AllottedGeometry, OutDrawElements, LayerId + 1);
    }

    // 弹出裁剪区域
    OutDrawElements.PopClip();

    return MaxLayerId;
}

FReply UMADAGCanvasWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // 检查是否点击在边上
        FMAEdgeRenderData* ClickedEdge = FMADAGCanvasGeometry::FindEdgeAtPoint(EdgeRenderData, LocalPosition);
        if (ClickedEdge)
        {
            // 选中边
            SelectEdge(ClickedEdge->FromNodeId, ClickedEdge->ToNodeId);
            return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
        }

        // 开始拖拽画布
        CanvasState.bIsDraggingCanvas = true;
        CanvasState.CanvasDragStart = LocalPosition;
        CanvasState.CanvasDragStartOffset = CanvasState.ViewOffset;

        // 取消所有选中
        ClearSelection();

        return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget(), EFocusCause::Mouse);
    }

    // 其他鼠标按钮也拦截，防止事件穿透
    return FReply::Handled();
}

FReply UMADAGCanvasWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (CanvasState.bIsDraggingCanvas)
        {
            CanvasState.bIsDraggingCanvas = false;
            return FReply::Handled().ReleaseMouseCapture();
        }

        if (CanvasState.bIsDraggingEdge)
        {
            // 检查是否释放在某个节点的输入端口上
            FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
            FString TargetNodeId = FMADAGCanvasGeometry::FindInputPortAtPoint(
                NodeWidgets,
                LocalPosition,
                CanvasState.ZoomLevel,
                CanvasState.ViewOffset);

            if (!TargetNodeId.IsEmpty() && TargetNodeId != CanvasState.DragSourceNodeId)
            {
                // 创建边
                if (GraphModel)
                {
                    if (GraphModel->AddEdge(CanvasState.DragSourceNodeId, TargetNodeId))
                    {
                        CreateEdge(CanvasState.DragSourceNodeId, TargetNodeId);
                        OnEdgeCreated.Broadcast(CanvasState.DragSourceNodeId, TargetNodeId);
                    }
                }
            }

            // 清除高亮
            if (!CanvasState.HighlightedTargetNodeId.IsEmpty())
            {
                UMATaskNodeWidget* TargetWidget = GetNodeWidget(CanvasState.HighlightedTargetNodeId);
                if (TargetWidget)
                {
                    TargetWidget->SetInputPortHighlighted(false);
                }
                CanvasState.HighlightedTargetNodeId.Empty();
            }

            CanvasState.bIsDraggingEdge = false;
            CanvasState.DragSourceNodeId.Empty();
            return FReply::Handled().ReleaseMouseCapture();
        }
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UMADAGCanvasWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

    if (CanvasState.bIsDraggingCanvas)
    {
        // 计算拖拽偏移
        FVector2D Delta = LocalPosition - CanvasState.CanvasDragStart;
        SetViewOffset(CanvasState.CanvasDragStartOffset + Delta);
        return FReply::Handled();
    }

    if (CanvasState.bIsDraggingEdge)
    {
        // 更新预览线终点
        CanvasState.DragEdgeEnd = LocalPosition;

        // 检查是否悬浮在某个节点的输入端口上
        FString NewTargetNodeId = FMADAGCanvasGeometry::FindInputPortAtPoint(
            NodeWidgets,
            LocalPosition,
            CanvasState.ZoomLevel,
            CanvasState.ViewOffset);

        // 更新高亮状态
        if (NewTargetNodeId != CanvasState.HighlightedTargetNodeId)
        {
            // 取消旧高亮
            if (!CanvasState.HighlightedTargetNodeId.IsEmpty())
            {
                UMATaskNodeWidget* OldTarget = GetNodeWidget(CanvasState.HighlightedTargetNodeId);
                if (OldTarget)
                {
                    OldTarget->SetInputPortHighlighted(false);
                }
            }

            // 设置新高亮
            if (!NewTargetNodeId.IsEmpty() && NewTargetNodeId != CanvasState.DragSourceNodeId)
            {
                UMATaskNodeWidget* NewTarget = GetNodeWidget(NewTargetNodeId);
                if (NewTarget)
                {
                    NewTarget->SetInputPortHighlighted(true);
                }
            }

            CanvasState.HighlightedTargetNodeId = NewTargetNodeId;
        }

        return FReply::Handled();
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UMADAGCanvasWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 缩放
    float WheelDelta = InMouseEvent.GetWheelDelta();
    float ZoomDelta = WheelDelta * 0.1f;
    float NewZoom = FMath::Clamp(CanvasState.ZoomLevel + ZoomDelta, CanvasState.MinZoom, CanvasState.MaxZoom);

    if (NewZoom != CanvasState.ZoomLevel)
    {
        // 以鼠标位置为中心缩放
        FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        FVector2D CanvasPos = ScreenToCanvas(LocalPosition);

        CanvasState.ZoomLevel = NewZoom;

        // 调整偏移以保持鼠标位置不变
        FVector2D NewScreenPos = CanvasToScreen(CanvasPos);
        CanvasState.ViewOffset += LocalPosition - NewScreenPos;

        UpdateNodePositions();
        UpdateEdgePositions();
    }

    return FReply::Handled();
}

FReply UMADAGCanvasWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        // 取消连线拖拽
        if (CanvasState.bIsDraggingEdge)
        {
            // 清除高亮
            if (!CanvasState.HighlightedTargetNodeId.IsEmpty())
            {
                UMATaskNodeWidget* TargetWidget = GetNodeWidget(CanvasState.HighlightedTargetNodeId);
                if (TargetWidget)
                {
                    TargetWidget->SetInputPortHighlighted(false);
                }
                CanvasState.HighlightedTargetNodeId.Empty();
            }

            CanvasState.bIsDraggingEdge = false;
            CanvasState.DragSourceNodeId.Empty();
            return FReply::Handled();
        }

        // 取消选中
        ClearSelection();
        return FReply::Handled();
    }

    // Delete 或 Backspace 键删除选中的元素
    if (InKeyEvent.GetKey() == EKeys::Delete || InKeyEvent.GetKey() == EKeys::BackSpace)
    {
        DeleteSelectedElement();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

//=============================================================================
// UI 构建
//=============================================================================

void UMADAGCanvasWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMADAGCanvas, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }

    UE_LOG(LogMADAGCanvas, Verbose, TEXT("BuildUI: Starting UI construction..."));

    // 在构建 UI 之前先从主题获取颜色
    if (!Theme)
    {
        Theme = NewObject<UMAUITheme>();
    }
    CanvasBackgroundColor = Theme->CanvasBackgroundColor;

    // 创建背景边框 - 使用圆角效果
    CanvasBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CanvasBackground"));
    MARoundedBorderUtils::ApplyRoundedCorners(CanvasBackground, CanvasBackgroundColor, MARoundedBorderUtils::DefaultPanelCornerRadius);
    WidgetTree->RootWidget = CanvasBackground;
    
    // 启用裁剪 - 确保内容不会溢出边界
    CanvasBackground->SetClipping(EWidgetClipping::ClipToBoundsAlways);

    // 创建节点容器
    NodeContainer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NodeContainer"));
    CanvasBackground->AddChild(NodeContainer);
    
    // 节点容器也启用裁剪
    NodeContainer->SetClipping(EWidgetClipping::ClipToBoundsAlways);

    UE_LOG(LogMADAGCanvas, Verbose, TEXT("BuildUI: UI construction completed"));
}

void UMADAGCanvasWidget::UpdateNodePositions()
{
    for (auto& Pair : NodeWidgets)
    {
        UMATaskNodeWidget* Widget = Pair.Value;
        if (!Widget) continue;

        FMATaskNodeData Data = Widget->GetNodeData();
        FVector2D ScreenPos = CanvasToScreen(Data.CanvasPosition);

        UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot);
        if (Slot)
        {
            Slot->SetPosition(ScreenPos);
        }
    }
}


//=============================================================================
// 绘制
//=============================================================================

void UMADAGCanvasWidget::DrawEdges(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    for (const FMAEdgeRenderData& Edge : EdgeRenderData)
    {
        DrawEdge(Edge, AllottedGeometry, OutDrawElements, LayerId);
    }
}

void UMADAGCanvasWidget::DrawEdge(const FMAEdgeRenderData& Edge, const FGeometry& AllottedGeometry,
                                   FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // Edge.StartPoint 和 Edge.EndPoint 已经是屏幕坐标了
    FVector2D StartScreen = Edge.StartPoint;
    FVector2D EndScreen = Edge.EndPoint;

    // 选择颜色
    FLinearColor EdgeColor = Edge.bIsHighlighted ? HighlightedEdgeColor : DefaultEdgeColor;

    // 绘制线条
    TArray<FVector2D> Points;
    Points.Add(StartScreen);
    Points.Add(EndScreen);

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        Points,
        ESlateDrawEffect::None,
        EdgeColor,
        true,
        EdgeThickness
    );

    // 绘制箭头
    DrawArrow(StartScreen, EndScreen, EdgeColor, AllottedGeometry, OutDrawElements, LayerId);
}

void UMADAGCanvasWidget::DrawPreviewEdge(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 绘制预览线
    TArray<FVector2D> Points;
    Points.Add(CanvasState.DragEdgeStart);
    Points.Add(CanvasState.DragEdgeEnd);

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        Points,
        ESlateDrawEffect::None,
        PreviewEdgeColor,
        true,
        EdgeThickness
    );

    // 绘制箭头
    DrawArrow(CanvasState.DragEdgeStart, CanvasState.DragEdgeEnd, PreviewEdgeColor, AllottedGeometry, OutDrawElements, LayerId);
}

void UMADAGCanvasWidget::DrawArrow(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color,
                                    const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 计算方向
    FVector2D Direction = End - Start;
    float Length = Direction.Size();
    if (Length < 1.0f) return;

    Direction /= Length;

    // 箭头位置 (在终点附近)
    FVector2D ArrowTip = End;
    FVector2D ArrowBase = End - Direction * ArrowSize;

    // 计算箭头两侧的点
    FVector2D Perpendicular(-Direction.Y, Direction.X);
    FVector2D ArrowLeft = ArrowBase + Perpendicular * (ArrowSize * 0.5f);
    FVector2D ArrowRight = ArrowBase - Perpendicular * (ArrowSize * 0.5f);

    // 绘制箭头三角形
    TArray<FVector2D> ArrowPoints;
    ArrowPoints.Add(ArrowTip);
    ArrowPoints.Add(ArrowLeft);
    ArrowPoints.Add(ArrowRight);
    ArrowPoints.Add(ArrowTip); // 闭合

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        ArrowPoints,
        ESlateDrawEffect::None,
        Color,
        true,
        EdgeThickness
    );
}

//=============================================================================
// 节点事件处理
//=============================================================================

void UMADAGCanvasWidget::OnNodeWidgetSelected(const FString& NodeId)
{
    SelectNode(NodeId);
}

void UMADAGCanvasWidget::OnNodeWidgetDragStarted(const FString& NodeId, FVector2D StartPosition)
{
    UE_LOG(LogMADAGCanvas, Verbose, TEXT("Node drag started: %s"), *NodeId);
}

void UMADAGCanvasWidget::OnNodeWidgetDragging(const FString& NodeId, FVector2D CurrentPosition)
{
    MoveNode(NodeId, CurrentPosition);
}

void UMADAGCanvasWidget::OnNodeWidgetDragEnded(const FString& NodeId, FVector2D EndPosition)
{
    UE_LOG(LogMADAGCanvas, Verbose, TEXT("Node drag ended: %s at (%.1f, %.1f)"), *NodeId, EndPosition.X, EndPosition.Y);
    
    // 更新模型中的位置
    if (GraphModel)
    {
        GraphModel->UpdateNodePosition(NodeId, EndPosition);
    }
}

void UMADAGCanvasWidget::OnNodePortDragStarted(const FString& NodeId, bool bIsOutputPort, FVector2D PortPosition)
{
    if (!bIsOutputPort)
    {
        // 只能从输出端口开始拖拽
        return;
    }

    CanvasState.bIsDraggingEdge = true;
    CanvasState.DragSourceNodeId = NodeId;
    
    // 获取输出端口的屏幕位置
    UMATaskNodeWidget* Widget = GetNodeWidget(NodeId);
    if (Widget)
    {
        CanvasState.DragEdgeStart = CanvasToScreen(Widget->GetOutputPortPosition());
    }
    else
    {
        CanvasState.DragEdgeStart = PortPosition;
    }
    CanvasState.DragEdgeEnd = CanvasState.DragEdgeStart;

    UE_LOG(LogMADAGCanvas, Verbose, TEXT("Port drag started from: %s"), *NodeId);
}

void UMADAGCanvasWidget::OnNodePortDragReleased(const FString& NodeId, bool bIsInputPort, FVector2D PortPosition)
{
    // 这个回调来自节点 Widget，但实际的连线创建在 NativeOnMouseButtonUp 中处理
    UE_LOG(LogMADAGCanvas, Verbose, TEXT("Port drag released on: %s (input: %d)"), *NodeId, bIsInputPort);
}

void UMADAGCanvasWidget::OnNodeWidgetDoubleClicked(const FString& NodeId)
{
    OnNodeEditRequested.Broadcast(NodeId);
}

void UMADAGCanvasWidget::OnNodeWidgetRightClicked(const FString& NodeId, FVector2D ScreenPosition)
{
    // 右键点击节点 - 选中节点 (上下文菜单已禁用)
    SelectNode(NodeId);
}

//=============================================================================
// 上下文菜单 (已禁用 - 使用 Backspace 删除)
//=============================================================================

void UMADAGCanvasWidget::ShowNodeContextMenu(const FString& NodeId, FVector2D ScreenPosition)
{
    // 上下文菜单已禁用，改为选中节点
    SelectNode(NodeId);
    UE_LOG(LogMADAGCanvas, Verbose, TEXT("ShowNodeContextMenu: Selected node %s (context menu disabled)"), *NodeId);
}

void UMADAGCanvasWidget::ShowEdgeContextMenu(const FString& FromNodeId, const FString& ToNodeId, FVector2D ScreenPosition)
{
    // 上下文菜单已禁用，改为选中边
    SelectEdge(FromNodeId, ToNodeId);
    UE_LOG(LogMADAGCanvas, Verbose, TEXT("ShowEdgeContextMenu: Selected edge %s -> %s (context menu disabled)"), *FromNodeId, *ToNodeId);
}

void UMADAGCanvasWidget::OnContextMenuItemClicked(const FString& ItemId)
{
    // 上下文菜单已禁用
}

void UMADAGCanvasWidget::OnContextMenuClosed()
{
    // 上下文菜单已禁用
}

//=============================================================================
// 模型事件处理
//=============================================================================

void UMADAGCanvasWidget::OnModelDataChanged()
{
    RefreshFromModel();
}

//=============================================================================
// 辅助方法
//=============================================================================

FVector2D UMADAGCanvasWidget::CanvasToScreen(FVector2D CanvasPos) const
{
    return FMADAGCanvasGeometry::CanvasToScreen(CanvasPos, CanvasState.ZoomLevel, CanvasState.ViewOffset);
}

FVector2D UMADAGCanvasWidget::ScreenToCanvas(FVector2D ScreenPos) const
{
    return FMADAGCanvasGeometry::ScreenToCanvas(ScreenPos, CanvasState.ZoomLevel, CanvasState.ViewOffset);
}

void UMADAGCanvasWidget::AutoLayoutNodes()
{
    if (!GraphModel || NodeWidgets.IsEmpty())
    {
        return;
    }

    const FMATaskGraphData Data = GraphModel->GetWorkingData();
    const TMap<FString, FVector2D> Layout = FMADAGCanvasAutoLayout::BuildTopologicalLayout(Data);

    for (const TPair<FString, FVector2D>& Pair : Layout)
    {
        UMATaskNodeWidget* Widget = GetNodeWidget(Pair.Key);
        if (!Widget)
        {
            continue;
        }

        FMATaskNodeData NodeData = Widget->GetNodeData();
        NodeData.CanvasPosition = Pair.Value;
        Widget->SetNodeData(NodeData);

        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
        {
            Slot->SetPosition(CanvasToScreen(Pair.Value));
        }

        GraphModel->UpdateNodePosition(Pair.Key, Pair.Value);
    }

    UpdateEdgePositions();

    UE_LOG(LogMADAGCanvas, Log, TEXT("Topological layout completed: %d nodes"), NodeWidgets.Num());
}
