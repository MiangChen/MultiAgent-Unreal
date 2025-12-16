// MATaskNodeWidget.cpp
// 任务节点 Widget - DAG 画布中的单个任务节点可视化组件
// Requirements: 4.1, 5.1, 5.2

#include "MATaskNodeWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"

DEFINE_LOG_CATEGORY(LogMATaskNodeWidget);

//=============================================================================
// 构造函数
//=============================================================================

UMATaskNodeWidget::UMATaskNodeWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 确保 WidgetTree 存在
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
}

//=============================================================================
// 初始化
//=============================================================================

void UMATaskNodeWidget::InitializeNode(const FMATaskNodeData& Data)
{
    NodeData = Data;
    UpdateDisplay();
    
    UE_LOG(LogMATaskNodeWidget, Verbose, TEXT("InitializeNode: %s - %s"), *Data.TaskId, *Data.Description);
}

void UMATaskNodeWidget::SetNodeData(const FMATaskNodeData& Data)
{
    NodeData = Data;
    UpdateDisplay();
}

void UMATaskNodeWidget::UpdateDisplay()
{
    if (TaskIdText)
    {
        TaskIdText->SetText(FText::FromString(NodeData.TaskId));
    }
    
    if (DescriptionText)
    {
        // 截断过长的描述
        FString DisplayDesc = NodeData.Description;
        if (DisplayDesc.Len() > 50)
        {
            DisplayDesc = DisplayDesc.Left(47) + TEXT("...");
        }
        DescriptionText->SetText(FText::FromString(DisplayDesc));
    }
    
    if (LocationText)
    {
        FString LocationDisplay = FString::Printf(TEXT("📍 %s"), *NodeData.Location);
        LocationText->SetText(FText::FromString(LocationDisplay));
    }
    
    // 更新 Tooltip
    SetToolTipText(FText::FromString(GenerateTooltipText()));
}

//=============================================================================
// 端口位置
//=============================================================================

FVector2D UMATaskNodeWidget::GetInputPortLocalPosition() const
{
    // 输入端口在节点顶部中央
    return FVector2D(NodeSize.X / 2.0f, 0.0f);
}

FVector2D UMATaskNodeWidget::GetOutputPortLocalPosition() const
{
    // 输出端口在节点底部中央
    return FVector2D(NodeSize.X / 2.0f, NodeSize.Y);
}

FVector2D UMATaskNodeWidget::GetInputPortPosition() const
{
    // 获取节点在画布中的位置，加上端口的本地偏移
    // 注意：这里返回的是画布坐标，端口偏移不受缩放影响
    // 因为节点 Widget 本身不缩放，只是位置改变
    FVector2D NodePosition = NodeData.CanvasPosition;
    // 端口偏移需要除以缩放级别，因为节点 Widget 不缩放
    // 但这里我们返回画布坐标，所以直接加上本地偏移
    return NodePosition + GetInputPortLocalPosition();
}

FVector2D UMATaskNodeWidget::GetOutputPortPosition() const
{
    FVector2D NodePosition = NodeData.CanvasPosition;
    return NodePosition + GetOutputPortLocalPosition();
}

//=============================================================================
// 交互状态
//=============================================================================

void UMATaskNodeWidget::SetSelected(bool bInSelected)
{
    if (bIsSelected != bInSelected)
    {
        bIsSelected = bInSelected;
        UpdateBorderColor();
        
        if (bIsSelected)
        {
            OnNodeSelected.Broadcast(NodeData.TaskId);
        }
    }
}

void UMATaskNodeWidget::SetHighlighted(bool bInHighlighted)
{
    if (bIsHighlighted != bInHighlighted)
    {
        bIsHighlighted = bInHighlighted;
        UpdateBorderColor();
    }
}

void UMATaskNodeWidget::SetInputPortHighlighted(bool bHighlighted)
{
    if (bInputPortHighlighted != bHighlighted)
    {
        bInputPortHighlighted = bHighlighted;
        UpdatePortColors();
    }
}

void UMATaskNodeWidget::SetOutputPortHighlighted(bool bHighlighted)
{
    if (bOutputPortHighlighted != bHighlighted)
    {
        bOutputPortHighlighted = bHighlighted;
        UpdatePortColors();
    }
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMATaskNodeWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        BuildUI();
    }
}

void UMATaskNodeWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 更新显示
    UpdateDisplay();
    
    UE_LOG(LogMATaskNodeWidget, Verbose, TEXT("NativeConstruct: Node %s"), *NodeData.TaskId);
}

TSharedRef<SWidget> UMATaskNodeWidget::RebuildWidget()
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


FReply UMATaskNodeWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // 检查是否点击在端口上
        if (IsPointOnOutputPort(LocalPosition))
        {
            // 开始从输出端口拖拽连线
            bIsDraggingPort = true;
            bDraggingOutputPort = true;
            DragStartPosition = GetOutputPortPosition();
            
            OnPortDragStarted.Broadcast(NodeData.TaskId, true, DragStartPosition);
            
            UE_LOG(LogMATaskNodeWidget, Verbose, TEXT("Started port drag from output: %s"), *NodeData.TaskId);
            return FReply::Handled().CaptureMouse(TakeWidget());
        }
        else if (IsPointOnInputPort(LocalPosition))
        {
            // 输入端口不能开始拖拽连线 (Requirements: 6.2)
            UE_LOG(LogMATaskNodeWidget, Verbose, TEXT("Input port click ignored: %s"), *NodeData.TaskId);
            return FReply::Handled();
        }
        else
        {
            // 开始拖拽节点
            bIsDraggingNode = true;
            DragStartPosition = LocalPosition;
            
            // 选中节点
            SetSelected(true);
            
            OnDragStarted.Broadcast(NodeData.TaskId, NodeData.CanvasPosition);
            
            UE_LOG(LogMATaskNodeWidget, Verbose, TEXT("Started node drag: %s"), *NodeData.TaskId);
            return FReply::Handled().CaptureMouse(TakeWidget());
        }
    }
    else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        // 右键点击 - 显示上下文菜单
        FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
        OnRightClicked.Broadcast(NodeData.TaskId, ScreenPosition);
        
        UE_LOG(LogMATaskNodeWidget, Verbose, TEXT("Right click on node: %s"), *NodeData.TaskId);
        return FReply::Handled();
    }
    
    // 拦截所有鼠标按钮事件，防止穿透到游戏场景
    return FReply::Handled();
}

FReply UMATaskNodeWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (bIsDraggingNode)
        {
            bIsDraggingNode = false;
            OnDragEnded.Broadcast(NodeData.TaskId, NodeData.CanvasPosition);
            
            UE_LOG(LogMATaskNodeWidget, Verbose, TEXT("Ended node drag: %s"), *NodeData.TaskId);
            return FReply::Handled().ReleaseMouseCapture();
        }
        else if (bIsDraggingPort)
        {
            bIsDraggingPort = false;
            
            // 端口拖拽释放 - 由画布处理连线创建
            FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
            OnPortDragReleased.Broadcast(NodeData.TaskId, false, LocalPosition);
            
            UE_LOG(LogMATaskNodeWidget, Verbose, TEXT("Ended port drag: %s"), *NodeData.TaskId);
            return FReply::Handled().ReleaseMouseCapture();
        }
    }
    
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UMATaskNodeWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    
    if (bIsDraggingNode)
    {
        // 计算拖拽偏移
        FVector2D Delta = LocalPosition - DragStartPosition;
        FVector2D NewPosition = NodeData.CanvasPosition + Delta;
        
        OnDragging.Broadcast(NodeData.TaskId, NewPosition);
        
        return FReply::Handled();
    }
    else if (bIsDraggingPort)
    {
        // 端口拖拽中 - 由画布处理预览线绘制
        return FReply::Handled();
    }
    else
    {
        // 检查端口悬浮高亮
        bool bOnInput = IsPointOnInputPort(LocalPosition);
        bool bOnOutput = IsPointOnOutputPort(LocalPosition);
        
        SetInputPortHighlighted(bOnInput);
        SetOutputPortHighlighted(bOnOutput);
    }
    
    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UMATaskNodeWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        OnDoubleClicked.Broadcast(NodeData.TaskId);
        
        UE_LOG(LogMATaskNodeWidget, Verbose, TEXT("Double click on node: %s"), *NodeData.TaskId);
        return FReply::Handled();
    }
    
    return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UMATaskNodeWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
    
    SetHighlighted(true);
}

void UMATaskNodeWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
    
    SetHighlighted(false);
    SetInputPortHighlighted(false);
    SetOutputPortHighlighted(false);
}


//=============================================================================
// UI 构建
//=============================================================================

void UMATaskNodeWidget::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMATaskNodeWidget, Error, TEXT("BuildUI: WidgetTree is null!"));
        return;
    }
    
    UE_LOG(LogMATaskNodeWidget, Verbose, TEXT("BuildUI: Starting UI construction..."));

    // 创建根 CanvasPanel (用于定位端口)
    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMATaskNodeWidget, Error, TEXT("BuildUI: Failed to create RootCanvas!"));
        return;
    }
    WidgetTree->RootWidget = RootCanvas;

    // 创建 SizeBox 控制节点大小
    USizeBox* NodeSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NodeSizeBox"));
    NodeSizeBox->SetWidthOverride(NodeSize.X);
    NodeSizeBox->SetHeightOverride(NodeSize.Y);
    
    UCanvasPanelSlot* SizeBoxSlot = RootCanvas->AddChildToCanvas(NodeSizeBox);
    SizeBoxSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
    SizeBoxSlot->SetPosition(FVector2D(0, 0));
    SizeBoxSlot->SetAutoSize(true);

    // 创建节点背景边框
    NodeBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NodeBackground"));
    NodeBackground->SetBrushColor(DefaultBackgroundColor);
    NodeBackground->SetPadding(FMargin(10.0f, 15.0f, 10.0f, 15.0f));
    NodeSizeBox->AddChild(NodeBackground);

    // 创建内容垂直布局
    ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
    NodeBackground->AddChild(ContentBox);

    // 任务 ID 文本 (标题)
    TaskIdText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TaskIdText"));
    TaskIdText->SetText(FText::FromString(TEXT("Task ID")));
    FSlateFontInfo TitleFont = TaskIdText->GetFont();
    TitleFont.Size = 12;
    TaskIdText->SetFont(TitleFont);
    TaskIdText->SetColorAndOpacity(FSlateColor(TitleTextColor));
    
    UVerticalBoxSlot* TitleSlot = ContentBox->AddChildToVerticalBox(TaskIdText);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 5));

    // 描述文本
    DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
    DescriptionText->SetText(FText::FromString(TEXT("Description")));
    FSlateFontInfo DescFont = DescriptionText->GetFont();
    DescFont.Size = 10;
    DescriptionText->SetFont(DescFont);
    DescriptionText->SetColorAndOpacity(FSlateColor(DescriptionTextColor));
    DescriptionText->SetAutoWrapText(true);
    
    UVerticalBoxSlot* DescSlot = ContentBox->AddChildToVerticalBox(DescriptionText);
    DescSlot->SetPadding(FMargin(0, 0, 0, 5));
    DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    // 位置文本
    LocationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LocationText"));
    LocationText->SetText(FText::FromString(TEXT("📍 Location")));
    FSlateFontInfo LocFont = LocationText->GetFont();
    LocFont.Size = 9;
    LocationText->SetFont(LocFont);
    LocationText->SetColorAndOpacity(FSlateColor(LocationTextColor));
    
    UVerticalBoxSlot* LocSlot = ContentBox->AddChildToVerticalBox(LocationText);
    LocSlot->SetPadding(FMargin(0, 0, 0, 0));

    // 创建输入端口 (顶部中央)
    InputPort = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InputPort"));
    InputPort->SetBrushColor(DefaultPortColor);
    InputPort->SetDesiredSizeScale(FVector2D(1.0f, 1.0f));
    
    UCanvasPanelSlot* InputPortSlot = RootCanvas->AddChildToCanvas(InputPort);
    InputPortSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
    // 端口位置: 顶部中央，向上偏移半个端口高度
    InputPortSlot->SetPosition(FVector2D(NodeSize.X / 2.0f - PortRadius, -PortRadius));
    InputPortSlot->SetSize(FVector2D(PortRadius * 2, PortRadius * 2));
    InputPortSlot->SetAutoSize(false);

    // 创建输出端口 (底部中央)
    OutputPort = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OutputPort"));
    OutputPort->SetBrushColor(DefaultPortColor);
    OutputPort->SetDesiredSizeScale(FVector2D(1.0f, 1.0f));
    
    UCanvasPanelSlot* OutputPortSlot = RootCanvas->AddChildToCanvas(OutputPort);
    OutputPortSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
    // 端口位置: 底部中央，向下偏移半个端口高度
    OutputPortSlot->SetPosition(FVector2D(NodeSize.X / 2.0f - PortRadius, NodeSize.Y - PortRadius));
    OutputPortSlot->SetSize(FVector2D(PortRadius * 2, PortRadius * 2));
    OutputPortSlot->SetAutoSize(false);

    UE_LOG(LogMATaskNodeWidget, Verbose, TEXT("BuildUI: UI construction completed"));
}

void UMATaskNodeWidget::UpdateBorderColor()
{
    if (!NodeBackground)
    {
        return;
    }
    
    if (bIsSelected)
    {
        NodeBackground->SetBrushColor(SelectedBackgroundColor);
    }
    else if (bIsHighlighted)
    {
        NodeBackground->SetBrushColor(HighlightedBackgroundColor);
    }
    else
    {
        NodeBackground->SetBrushColor(DefaultBackgroundColor);
    }
}

void UMATaskNodeWidget::UpdatePortColors()
{
    if (InputPort)
    {
        InputPort->SetBrushColor(bInputPortHighlighted ? HighlightedPortColor : DefaultPortColor);
    }
    
    if (OutputPort)
    {
        OutputPort->SetBrushColor(bOutputPortHighlighted ? HighlightedPortColor : DefaultPortColor);
    }
}

FString UMATaskNodeWidget::GenerateTooltipText() const
{
    FString Tooltip;
    
    Tooltip += FString::Printf(TEXT("Task ID: %s\n"), *NodeData.TaskId);
    Tooltip += FString::Printf(TEXT("Description: %s\n"), *NodeData.Description);
    Tooltip += FString::Printf(TEXT("Location: %s\n"), *NodeData.Location);
    
    if (NodeData.RequiredSkills.Num() > 0)
    {
        Tooltip += TEXT("\nRequired Skills:\n");
        for (const FMARequiredSkill& Skill : NodeData.RequiredSkills)
        {
            Tooltip += FString::Printf(TEXT("  - %s (x%d)\n"), *Skill.SkillName, Skill.AssignedRobotCount);
        }
    }
    
    if (NodeData.Produces.Num() > 0)
    {
        Tooltip += TEXT("\nProduces:\n");
        for (const FString& Product : NodeData.Produces)
        {
            Tooltip += FString::Printf(TEXT("  - %s\n"), *Product);
        }
    }
    
    return Tooltip;
}

//=============================================================================
// 端口命中检测
//=============================================================================

bool UMATaskNodeWidget::IsPointOnInputPort(const FVector2D& LocalPosition) const
{
    // 输入端口中心位置 (相对于节点)
    FVector2D PortCenter = FVector2D(NodeSize.X / 2.0f, 0.0f);
    
    // 计算距离
    float Distance = FVector2D::Distance(LocalPosition, PortCenter);
    
    return Distance <= PortHitRadius;
}

bool UMATaskNodeWidget::IsPointOnOutputPort(const FVector2D& LocalPosition) const
{
    // 输出端口中心位置 (相对于节点)
    FVector2D PortCenter = FVector2D(NodeSize.X / 2.0f, NodeSize.Y);
    
    // 计算距离
    float Distance = FVector2D::Distance(LocalPosition, PortCenter);
    
    return Distance <= PortHitRadius;
}
