// MADAGCanvasWidget.cpp
// DAG 画布 Widget 实现
// Requirements: 4.1, 4.2, 7.1, 7.4, 10.1, 10.2, 10.3

#include "MADAGCanvasWidget.h"
#include "MATaskNodeWidget.h"
#include "MATaskGraphModel.h"
#include "MAContextMenuWidget.h"
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
    if (SelectedNodeId == NodeId)
    {
        SelectedNodeId.Empty();
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
    SelectedNodeId.Empty();
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
    for (FMAEdgeRenderData& Edge : EdgeRenderData)
    {
        UMATaskNodeWidget* FromWidget = GetNodeWidget(Edge.FromNodeId);
        UMATaskNodeWidget* ToWidget = GetNodeWidget(Edge.ToNodeId);

        if (FromWidget && ToWidget)
        {
            // 获取节点的画布位置
            FMATaskNodeData FromData = FromWidget->GetNodeData();
            FMATaskNodeData ToData = ToWidget->GetNodeData();
            
            // 计算端口的屏幕位置
            // 节点屏幕位置 = 画布位置 * 缩放 + 偏移
            // 端口屏幕位置 = 节点屏幕位置 + 端口本地偏移 (不缩放)
            FVector2D FromNodeScreen = CanvasToScreen(FromData.CanvasPosition);
            FVector2D ToNodeScreen = CanvasToScreen(ToData.CanvasPosition);
            
            // 端口本地偏移不受缩放影响，因为节点 Widget 本身不缩放
            FVector2D OutputPortLocal = FromWidget->GetOutputPortLocalPosition();
            FVector2D InputPortLocal = ToWidget->GetInputPortLocalPosition();
            
            // 存储屏幕坐标 (而不是画布坐标)
            Edge.StartPoint = FromNodeScreen + OutputPortLocal;
            Edge.EndPoint = ToNodeScreen + InputPortLocal;
        }
    }
}

void UMADAGCanvasWidget::ClearAllEdges()
{
    EdgeRenderData.Empty();
}


//=============================================================================
// 视图操作
//=============================================================================

void UMADAGCanvasWidget::SetViewOffset(FVector2D Offset)
{
    ViewOffset = Offset;
    UpdateNodePositions();
    UpdateEdgePositions();
}

void UMADAGCanvasWidget::SetZoomLevel(float Zoom)
{
    ZoomLevel = FMath::Clamp(Zoom, MinZoom, MaxZoom);
    UpdateNodePositions();
    UpdateEdgePositions();
}

void UMADAGCanvasWidget::ResetView()
{
    ViewOffset = FVector2D::ZeroVector;
    ZoomLevel = 1.0f;
    UpdateNodePositions();
    UpdateEdgePositions();
}

//=============================================================================
// 选择
//=============================================================================

void UMADAGCanvasWidget::SelectNode(const FString& NodeId)
{
    // 取消之前的选中
    if (!SelectedNodeId.IsEmpty() && SelectedNodeId != NodeId)
    {
        UMATaskNodeWidget* OldWidget = GetNodeWidget(SelectedNodeId);
        if (OldWidget)
        {
            OldWidget->SetSelected(false);
        }
    }

    SelectedNodeId = NodeId;

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
    if (!SelectedNodeId.IsEmpty())
    {
        UMATaskNodeWidget* Widget = GetNodeWidget(SelectedNodeId);
        if (Widget)
        {
            Widget->SetSelected(false);
        }
        SelectedNodeId.Empty();
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
    SelectedEdgeFrom = FromNodeId;
    SelectedEdgeTo = ToNodeId;
    
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
    if (!SelectedEdgeFrom.IsEmpty() || !SelectedEdgeTo.IsEmpty())
    {
        // 取消高亮
        for (FMAEdgeRenderData& Edge : EdgeRenderData)
        {
            Edge.bIsHighlighted = false;
        }
        
        SelectedEdgeFrom.Empty();
        SelectedEdgeTo.Empty();
    }
}

void UMADAGCanvasWidget::DeleteSelectedElement()
{
    // 优先删除选中的节点
    if (!SelectedNodeId.IsEmpty())
    {
        OnNodeDeleteRequested.Broadcast(SelectedNodeId);
        SelectedNodeId.Empty();
        return;
    }
    
    // 其次删除选中的边
    if (!SelectedEdgeFrom.IsEmpty() && !SelectedEdgeTo.IsEmpty())
    {
        OnEdgeDeleteRequested.Broadcast(SelectedEdgeFrom, SelectedEdgeTo);
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
    // 先绘制子 Widget
    int32 MaxLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, 
                                          LayerId, InWidgetStyle, bParentEnabled);

    // 绘制边 (在节点下方)
    DrawEdges(AllottedGeometry, OutDrawElements, LayerId);

    // 绘制预览连线
    if (bIsDraggingEdge)
    {
        DrawPreviewEdge(AllottedGeometry, OutDrawElements, LayerId + 1);
    }

    return MaxLayerId;
}

FReply UMADAGCanvasWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // 检查是否点击在边上
        FMAEdgeRenderData* ClickedEdge = FindEdgeAtPoint(LocalPosition);
        if (ClickedEdge)
        {
            // 选中边
            SelectEdge(ClickedEdge->FromNodeId, ClickedEdge->ToNodeId);
            return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
        }

        // 开始拖拽画布
        bIsDraggingCanvas = true;
        CanvasDragStart = LocalPosition;
        CanvasDragStartOffset = ViewOffset;

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
        if (bIsDraggingCanvas)
        {
            bIsDraggingCanvas = false;
            return FReply::Handled().ReleaseMouseCapture();
        }

        if (bIsDraggingEdge)
        {
            // 检查是否释放在某个节点的输入端口上
            FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
            FString TargetNodeId = FindInputPortAtPoint(LocalPosition);

            if (!TargetNodeId.IsEmpty() && TargetNodeId != DragSourceNodeId)
            {
                // 创建边
                if (GraphModel)
                {
                    if (GraphModel->AddEdge(DragSourceNodeId, TargetNodeId))
                    {
                        CreateEdge(DragSourceNodeId, TargetNodeId);
                        OnEdgeCreated.Broadcast(DragSourceNodeId, TargetNodeId);
                    }
                }
            }

            // 清除高亮
            if (!HighlightedTargetNodeId.IsEmpty())
            {
                UMATaskNodeWidget* TargetWidget = GetNodeWidget(HighlightedTargetNodeId);
                if (TargetWidget)
                {
                    TargetWidget->SetInputPortHighlighted(false);
                }
                HighlightedTargetNodeId.Empty();
            }

            bIsDraggingEdge = false;
            DragSourceNodeId.Empty();
            return FReply::Handled().ReleaseMouseCapture();
        }
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UMADAGCanvasWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

    if (bIsDraggingCanvas)
    {
        // 计算拖拽偏移
        FVector2D Delta = LocalPosition - CanvasDragStart;
        SetViewOffset(CanvasDragStartOffset + Delta);
        return FReply::Handled();
    }

    if (bIsDraggingEdge)
    {
        // 更新预览线终点
        DragEdgeEnd = LocalPosition;

        // 检查是否悬浮在某个节点的输入端口上
        FString NewTargetNodeId = FindInputPortAtPoint(LocalPosition);

        // 更新高亮状态
        if (NewTargetNodeId != HighlightedTargetNodeId)
        {
            // 取消旧高亮
            if (!HighlightedTargetNodeId.IsEmpty())
            {
                UMATaskNodeWidget* OldTarget = GetNodeWidget(HighlightedTargetNodeId);
                if (OldTarget)
                {
                    OldTarget->SetInputPortHighlighted(false);
                }
            }

            // 设置新高亮
            if (!NewTargetNodeId.IsEmpty() && NewTargetNodeId != DragSourceNodeId)
            {
                UMATaskNodeWidget* NewTarget = GetNodeWidget(NewTargetNodeId);
                if (NewTarget)
                {
                    NewTarget->SetInputPortHighlighted(true);
                }
            }

            HighlightedTargetNodeId = NewTargetNodeId;
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
    float NewZoom = FMath::Clamp(ZoomLevel + ZoomDelta, MinZoom, MaxZoom);

    if (NewZoom != ZoomLevel)
    {
        // 以鼠标位置为中心缩放
        FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        FVector2D CanvasPos = ScreenToCanvas(LocalPosition);

        ZoomLevel = NewZoom;

        // 调整偏移以保持鼠标位置不变
        FVector2D NewScreenPos = CanvasToScreen(CanvasPos);
        ViewOffset += LocalPosition - NewScreenPos;

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
        if (bIsDraggingEdge)
        {
            // 清除高亮
            if (!HighlightedTargetNodeId.IsEmpty())
            {
                UMATaskNodeWidget* TargetWidget = GetNodeWidget(HighlightedTargetNodeId);
                if (TargetWidget)
                {
                    TargetWidget->SetInputPortHighlighted(false);
                }
                HighlightedTargetNodeId.Empty();
            }

            bIsDraggingEdge = false;
            DragSourceNodeId.Empty();
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

    // 创建背景边框
    CanvasBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CanvasBackground"));
    CanvasBackground->SetBrushColor(CanvasBackgroundColor);
    WidgetTree->RootWidget = CanvasBackground;

    // 创建节点容器
    NodeContainer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NodeContainer"));
    CanvasBackground->AddChild(NodeContainer);

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
    Points.Add(DragEdgeStart);
    Points.Add(DragEdgeEnd);

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
    DrawArrow(DragEdgeStart, DragEdgeEnd, PreviewEdgeColor, AllottedGeometry, OutDrawElements, LayerId);
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
        // 只能从输出端口开始拖拽 (Requirements: 6.2)
        return;
    }

    bIsDraggingEdge = true;
    DragSourceNodeId = NodeId;
    
    // 获取输出端口的屏幕位置
    UMATaskNodeWidget* Widget = GetNodeWidget(NodeId);
    if (Widget)
    {
        DragEdgeStart = CanvasToScreen(Widget->GetOutputPortPosition());
    }
    else
    {
        DragEdgeStart = PortPosition;
    }
    DragEdgeEnd = DragEdgeStart;

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
    // 模型数据变更时刷新画布
    // 注意：这里不直接调用 RefreshFromModel，因为可能导致循环
    // 只更新边位置
    UpdateEdgePositions();
}

//=============================================================================
// 辅助方法
//=============================================================================

FVector2D UMADAGCanvasWidget::CanvasToScreen(FVector2D CanvasPos) const
{
    return (CanvasPos * ZoomLevel) + ViewOffset;
}

FVector2D UMADAGCanvasWidget::ScreenToCanvas(FVector2D ScreenPos) const
{
    return (ScreenPos - ViewOffset) / ZoomLevel;
}

bool UMADAGCanvasWidget::IsPointOnEdge(const FVector2D& Point, const FMAEdgeRenderData& Edge, float Tolerance) const
{
    // Edge.StartPoint 和 Edge.EndPoint 已经是屏幕坐标了
    FVector2D StartScreen = Edge.StartPoint;
    FVector2D EndScreen = Edge.EndPoint;

    // 计算点到线段的距离
    FVector2D LineDir = EndScreen - StartScreen;
    float LineLength = LineDir.Size();
    if (LineLength < 1.0f) return false;

    LineDir /= LineLength;

    FVector2D PointToStart = Point - StartScreen;
    float Projection = FVector2D::DotProduct(PointToStart, LineDir);

    // 检查投影是否在线段范围内
    if (Projection < 0 || Projection > LineLength) return false;

    // 计算垂直距离
    FVector2D ClosestPoint = StartScreen + LineDir * Projection;
    float Distance = FVector2D::Distance(Point, ClosestPoint);

    return Distance <= Tolerance;
}

FMAEdgeRenderData* UMADAGCanvasWidget::FindEdgeAtPoint(const FVector2D& Point)
{
    for (FMAEdgeRenderData& Edge : EdgeRenderData)
    {
        if (IsPointOnEdge(Point, Edge))
        {
            return &Edge;
        }
    }
    return nullptr;
}

FString UMADAGCanvasWidget::FindInputPortAtPoint(const FVector2D& ScreenPoint) const
{
    const float PortHitRadius = 20.0f;

    for (const auto& Pair : NodeWidgets)
    {
        UMATaskNodeWidget* Widget = Pair.Value;
        if (!Widget) continue;

        // 获取输入端口的屏幕位置
        // 节点屏幕位置 + 端口本地偏移
        FMATaskNodeData NodeData = Widget->GetNodeData();
        FVector2D NodeScreenPos = CanvasToScreen(NodeData.CanvasPosition);
        FVector2D InputPortScreen = NodeScreenPos + Widget->GetInputPortLocalPosition();

        float Distance = FVector2D::Distance(ScreenPoint, InputPortScreen);
        if (Distance <= PortHitRadius)
        {
            return Pair.Key;
        }
    }

    return FString();
}

void UMADAGCanvasWidget::AutoLayoutNodes()
{
    if (NodeWidgets.Num() == 0) return;

    // 拓扑排序分层布局算法
    // 使用 Kahn 算法进行拓扑排序，同时计算每个节点的层级
    
    const float NodeSpacingX = 280.0f;  // 同层节点水平间距
    const float NodeSpacingY = 180.0f;  // 层与层之间的垂直间距
    const float StartX = 80.0f;
    const float StartY = 50.0f;

    // 构建邻接表和入度表
    TMap<FString, TArray<FString>> Adjacency;  // 节点 -> 后继节点列表
    TMap<FString, int32> InDegree;             // 节点 -> 入度
    TSet<FString> AllNodeIds;

    // 初始化所有节点
    for (const auto& Pair : NodeWidgets)
    {
        AllNodeIds.Add(Pair.Key);
        InDegree.Add(Pair.Key, 0);
        Adjacency.Add(Pair.Key, TArray<FString>());
    }

    // 根据边构建邻接表和入度
    for (const FMAEdgeRenderData& Edge : EdgeRenderData)
    {
        if (AllNodeIds.Contains(Edge.FromNodeId) && AllNodeIds.Contains(Edge.ToNodeId))
        {
            Adjacency[Edge.FromNodeId].Add(Edge.ToNodeId);
            InDegree[Edge.ToNodeId]++;
        }
    }

    // Kahn 算法 - 计算每个节点的层级
    TMap<FString, int32> NodeLevel;  // 节点 -> 层级 (0 为最顶层)
    TArray<FString> Queue;

    // 将所有入度为 0 的节点加入队列 (第 0 层)
    for (const auto& Pair : InDegree)
    {
        if (Pair.Value == 0)
        {
            Queue.Add(Pair.Key);
            NodeLevel.Add(Pair.Key, 0);
        }
    }

    // BFS 遍历，计算每个节点的层级
    int32 ProcessedCount = 0;
    while (Queue.Num() > 0)
    {
        FString CurrentNode = Queue[0];
        Queue.RemoveAt(0);
        ProcessedCount++;

        int32 CurrentLevel = NodeLevel[CurrentNode];

        // 遍历所有后继节点
        for (const FString& Successor : Adjacency[CurrentNode])
        {
            // 后继节点的层级至少是当前节点层级 + 1
            if (!NodeLevel.Contains(Successor))
            {
                NodeLevel.Add(Successor, CurrentLevel + 1);
            }
            else
            {
                // 取最大层级，确保所有前驱都在上层
                NodeLevel[Successor] = FMath::Max(NodeLevel[Successor], CurrentLevel + 1);
            }

            // 减少入度
            InDegree[Successor]--;
            if (InDegree[Successor] == 0)
            {
                Queue.Add(Successor);
            }
        }
    }

    // 处理可能存在的孤立节点或环路中的节点
    for (const FString& NodeId : AllNodeIds)
    {
        if (!NodeLevel.Contains(NodeId))
        {
            // 孤立节点或环路中的节点，放到最后一层
            NodeLevel.Add(NodeId, 0);
            UE_LOG(LogMADAGCanvas, Warning, TEXT("Node '%s' not reached by topological sort (isolated or in cycle)"), *NodeId);
        }
    }

    // 按层级分组节点
    TMap<int32, TArray<FString>> LevelNodes;  // 层级 -> 该层的节点列表
    int32 MaxLevel = 0;

    for (const auto& Pair : NodeLevel)
    {
        int32 Level = Pair.Value;
        if (!LevelNodes.Contains(Level))
        {
            LevelNodes.Add(Level, TArray<FString>());
        }
        LevelNodes[Level].Add(Pair.Key);
        MaxLevel = FMath::Max(MaxLevel, Level);
    }

    // 计算布局位置
    for (int32 Level = 0; Level <= MaxLevel; Level++)
    {
        if (!LevelNodes.Contains(Level)) continue;

        TArray<FString>& NodesInLevel = LevelNodes[Level];
        int32 NodeCount = NodesInLevel.Num();

        // 计算该层的起始 X 位置，使节点居中
        float TotalWidth = (NodeCount - 1) * NodeSpacingX;
        float LevelStartX = StartX + (MaxLevel > 0 ? 200.0f : 0.0f) - TotalWidth / 2.0f;
        
        // 确保不会出现负坐标
        LevelStartX = FMath::Max(LevelStartX, StartX);

        float Y = StartY + Level * NodeSpacingY;

        for (int32 i = 0; i < NodeCount; i++)
        {
            float X = LevelStartX + i * NodeSpacingX;
            FVector2D NewPosition(X, Y);
            MoveNode(NodesInLevel[i], NewPosition);
        }
    }

    UE_LOG(LogMADAGCanvas, Log, TEXT("Topological layout completed: %d nodes in %d levels"), 
           NodeWidgets.Num(), MaxLevel + 1);
}
