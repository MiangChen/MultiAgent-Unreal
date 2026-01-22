// MATaskGraphPreview.cpp
// 任务图预览组件实现 - 使用 NativePaint 绘制简化的 DAG 图
// 支持状态颜色显示和鼠标悬浮提示

#include "MATaskGraphPreview.h"
#include "../Core/MAUITheme.h"
#include "../Core/MARoundedBorderUtils.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Styling/SlateTypes.h"

//=============================================================================
// 日志类别
//=============================================================================
DEFINE_LOG_CATEGORY_STATIC(LogMATaskGraphPreview, Log, All);

//=============================================================================
// 构造函数
//=============================================================================

UMATaskGraphPreview::UMATaskGraphPreview(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

//=============================================================================
// UUserWidget 重写
//=============================================================================

void UMATaskGraphPreview::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UMATaskGraphPreview::NativeConstruct()
{
    Super::NativeConstruct();
}

TSharedRef<SWidget> UMATaskGraphPreview::RebuildWidget()
{
    BuildUI();
    return Super::RebuildWidget();
}


int32 UMATaskGraphPreview::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
                                        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, 
                                        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    // 缓存几何信息用于鼠标检测
    CachedGeometry = AllottedGeometry;
    
    // 先调用父类绘制
    LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
    
    // 如果有数据，绘制 DAG 图
    if (bHasData && NodeRenderData.Num() > 0)
    {
        DrawDAG(AllottedGeometry, OutDrawElements, LayerId);
        
        // 绘制工具提示 (在最上层)
        if (HoveredNodeIndex >= 0 && HoveredNodeIndex < NodeRenderData.Num())
        {
            DrawTooltip(AllottedGeometry, OutDrawElements, LayerId + 10);
        }
    }
    
    return LayerId;
}

FReply UMATaskGraphPreview::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 获取本地鼠标位置
    FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    LastMousePosition = LocalPos;
    
    // 查找悬浮的节点
    int32 NewHoveredIndex = FindNodeAtPosition(LocalPos);
    
    if (NewHoveredIndex != HoveredNodeIndex)
    {
        HoveredNodeIndex = NewHoveredIndex;
        // 触发重绘
        Invalidate(EInvalidateWidgetReason::Paint);
    }
    
    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UMATaskGraphPreview::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    // 清除悬浮状态
    if (HoveredNodeIndex >= 0)
    {
        HoveredNodeIndex = -1;
        Invalidate(EInvalidateWidgetReason::Paint);
    }
    
    Super::NativeOnMouseLeave(InMouseEvent);
}

//=============================================================================
// UI 构建
//=============================================================================

void UMATaskGraphPreview::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMATaskGraphPreview, Error, TEXT("BuildUI: WidgetTree is null"));
        return;
    }
    
    // 在构建 UI 之前先从主题获取颜色
    if (!Theme)
    {
        Theme = NewObject<UMAUITheme>();
    }
    BackgroundColor = Theme->CanvasBackgroundColor;
    
    // 创建根 SizeBox
    RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
    if (!RootSizeBox)
    {
        UE_LOG(LogMATaskGraphPreview, Error, TEXT("BuildUI: Failed to create RootSizeBox"));
        return;
    }
    RootSizeBox->SetMinDesiredHeight(PreviewHeight);
    
    // 设置为根 Widget
    WidgetTree->RootWidget = RootSizeBox;
    
    // 创建内容边框
    ContentBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ContentBorder"));
    if (!ContentBorder)
    {
        return;
    }
    ContentBorder->SetPadding(FMargin(8.0f));
    
    // 应用圆角效果
    MARoundedBorderUtils::ApplyRoundedCorners(ContentBorder, BackgroundColor, MARoundedBorderUtils::DefaultButtonCornerRadius);
    
    // 创建垂直布局容器
    UVerticalBox* ContentVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentVBox"));
    if (!ContentVBox)
    {
        return;
    }
    
    // 创建统计信息文本
    StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatsText"));
    if (StatsText)
    {
        StatsText->SetText(FText::FromString(TEXT("No data")));
        // 使用成员变量 TextColor（从 Theme 获取或使用 fallback 默认值）
        StatsText->SetColorAndOpacity(FSlateColor(TextColor));
        
        FSlateFontInfo StatsFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
        StatsText->SetFont(StatsFont);
        
        UVerticalBoxSlot* StatsSlot = ContentVBox->AddChildToVerticalBox(StatsText);
        StatsSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    }
    
    // 创建空状态提示 (当没有数据时显示)
    EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
    if (EmptyText)
    {
        EmptyText->SetText(FText::FromString(TEXT("Waiting for task graph...")));
        // 使用成员变量 HintTextColor（从 Theme 获取或使用 fallback 默认值）
        EmptyText->SetColorAndOpacity(FSlateColor(HintTextColor));
        EmptyText->SetJustification(ETextJustify::Center);
        
        FSlateFontInfo EmptyFont = FCoreStyle::GetDefaultFontStyle("Italic", 10);
        EmptyText->SetFont(EmptyFont);
        
        UVerticalBoxSlot* EmptySlot = ContentVBox->AddChildToVerticalBox(EmptyText);
        EmptySlot->SetHorizontalAlignment(HAlign_Center);
        EmptySlot->SetVerticalAlignment(VAlign_Center);
        EmptySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
    
    // 组装层级
    ContentBorder->SetContent(ContentVBox);
    RootSizeBox->SetContent(ContentBorder);
}


//=============================================================================
// 预览更新
//=============================================================================

void UMATaskGraphPreview::UpdatePreview(const FMATaskGraphData& Data)
{
    CurrentData = Data;
    bHasData = Data.Nodes.Num() > 0;
    HoveredNodeIndex = -1;  // 重置悬浮状态
    
    // 更新统计信息
    if (StatsText)
    {
        if (bHasData)
        {
            FString StatsStr = FString::Printf(TEXT("%d 节点, %d 边"), 
                CurrentData.Nodes.Num(), CurrentData.Edges.Num());
            StatsText->SetText(FText::FromString(StatsStr));
            // 使用成员变量 TextColor（从 Theme 获取或使用 fallback 默认值）
            StatsText->SetColorAndOpacity(FSlateColor(TextColor));
        }
        else
        {
            StatsText->SetText(FText::FromString(TEXT("No data")));
            // 使用成员变量 TextColor（从 Theme 获取或使用 fallback 默认值）
            StatsText->SetColorAndOpacity(FSlateColor(TextColor));
        }
    }
    
    // 更新空状态提示可见性
    if (EmptyText)
    {
        EmptyText->SetVisibility(bHasData ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
    
    // 清空渲染数据
    NodeRenderData.Empty();
    EdgeRenderData.Empty();
    
    // 预计算节点渲染数据
    if (bHasData)
    {
        // 使用拓扑排序计算层级
        TMap<FString, int32> NodeLevels;
        TMap<FString, int32> NodeIndexInLevel;
        TMap<int32, int32> LevelNodeCount;
        
        // 初始化所有节点层级为 0
        for (const FMATaskNodeData& Node : CurrentData.Nodes)
        {
            NodeLevels.Add(Node.TaskId, 0);
        }
        
        // 根据边更新层级 (多次迭代确保正确)
        for (int32 Iteration = 0; Iteration < CurrentData.Nodes.Num(); ++Iteration)
        {
            for (const FMATaskEdgeData& Edge : CurrentData.Edges)
            {
                int32* FromLevel = NodeLevels.Find(Edge.FromNodeId);
                int32* ToLevel = NodeLevels.Find(Edge.ToNodeId);
                if (FromLevel && ToLevel)
                {
                    *ToLevel = FMath::Max(*ToLevel, *FromLevel + 1);
                }
            }
        }
        
        // 统计每层节点数量
        for (const auto& Pair : NodeLevels)
        {
            int32& Count = LevelNodeCount.FindOrAdd(Pair.Value);
            NodeIndexInLevel.Add(Pair.Key, Count);
            Count++;
        }
        
        // 创建节点渲染数据
        for (const FMATaskNodeData& Node : CurrentData.Nodes)
        {
            FMAPreviewNodeRenderData RenderData;
            RenderData.NodeId = Node.TaskId;
            RenderData.Description = Node.Description;
            RenderData.Location = Node.Location;
            RenderData.Size = FVector2D(NodeWidth, NodeHeight);
            RenderData.Status = Node.Status;
            RenderData.Level = NodeLevels.FindRef(Node.TaskId);
            RenderData.IndexInLevel = NodeIndexInLevel.FindRef(Node.TaskId);
            
            NodeRenderData.Add(RenderData);
        }
        
        // 创建边渲染数据 (位置将在 CalculateLayout 中计算)
        for (const FMATaskEdgeData& Edge : CurrentData.Edges)
        {
            FMAPreviewEdgeRenderData EdgeData;
            EdgeData.EdgeType = Edge.EdgeType;
            EdgeRenderData.Add(EdgeData);
        }
    }
    
    UE_LOG(LogMATaskGraphPreview, Log, TEXT("UpdatePreview: %d nodes, %d edges"), 
        CurrentData.Nodes.Num(), CurrentData.Edges.Num());
}

void UMATaskGraphPreview::ClearPreview()
{
    CurrentData = FMATaskGraphData();
    bHasData = false;
    NodeRenderData.Empty();
    EdgeRenderData.Empty();
    HoveredNodeIndex = -1;
    
    if (StatsText)
    {
        StatsText->SetText(FText::FromString(TEXT("No data")));
        // 使用成员变量 TextColor（从 Theme 获取或使用 fallback 默认值）
        StatsText->SetColorAndOpacity(FSlateColor(TextColor));
    }
    
    if (EmptyText)
    {
        EmptyText->SetVisibility(ESlateVisibility::Visible);
    }
}


//=============================================================================
// 布局计算
//=============================================================================

void UMATaskGraphPreview::CalculateLayout(const FGeometry& AllottedGeometry)
{
    if (!bHasData || NodeRenderData.Num() == 0)
    {
        return;
    }
    
    FVector2D CanvasSize = AllottedGeometry.GetLocalSize();
    float AvailableWidth = CanvasSize.X - LeftMargin * 2;
    float AvailableHeight = CanvasSize.Y - TopMargin - 10.0f;
    
    // 按层级分组
    TMap<int32, TArray<int32>> LevelNodes;  // Level -> NodeRenderData indices
    int32 MaxLevel = 0;
    
    for (int32 i = 0; i < NodeRenderData.Num(); ++i)
    {
        int32 Level = NodeRenderData[i].Level;
        LevelNodes.FindOrAdd(Level).Add(i);
        MaxLevel = FMath::Max(MaxLevel, Level);
    }
    
    // 计算节点位置
    float LevelWidth = (MaxLevel > 0) ? AvailableWidth / (MaxLevel + 1) : AvailableWidth;
    
    for (auto& LevelPair : LevelNodes)
    {
        int32 Level = LevelPair.Key;
        TArray<int32>& Indices = LevelPair.Value;
        int32 NodesInLevel = Indices.Num();
        
        float LevelHeight = (NodesInLevel > 1) ? AvailableHeight / NodesInLevel : AvailableHeight;
        
        for (int32 i = 0; i < Indices.Num(); ++i)
        {
            int32 NodeIdx = Indices[i];
            float X = LeftMargin + Level * LevelWidth + LevelWidth / 2 - NodeWidth / 2;
            float Y = TopMargin + i * LevelHeight + LevelHeight / 2 - NodeHeight / 2;
            
            NodeRenderData[NodeIdx].Position = FVector2D(X, Y);
        }
    }
    
    // 计算边位置
    EdgeRenderData.Empty();
    for (const FMATaskEdgeData& Edge : CurrentData.Edges)
    {
        FMAPreviewEdgeRenderData EdgeData;
        EdgeData.EdgeType = Edge.EdgeType;
        
        // 查找源节点和目标节点
        FMAPreviewNodeRenderData* FromNode = nullptr;
        FMAPreviewNodeRenderData* ToNode = nullptr;
        
        for (FMAPreviewNodeRenderData& Node : NodeRenderData)
        {
            if (Node.NodeId == Edge.FromNodeId)
            {
                FromNode = &Node;
            }
            if (Node.NodeId == Edge.ToNodeId)
            {
                ToNode = &Node;
            }
        }
        
        if (FromNode && ToNode)
        {
            // 从源节点右边中心到目标节点左边中心
            EdgeData.StartPoint = FromNode->Position + FVector2D(NodeWidth, NodeHeight / 2);
            EdgeData.EndPoint = ToNode->Position + FVector2D(0, NodeHeight / 2);
            EdgeRenderData.Add(EdgeData);
        }
    }
}


//=============================================================================
// 绘制
//=============================================================================

void UMATaskGraphPreview::DrawDAG(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 需要在绘制前计算布局
    const_cast<UMATaskGraphPreview*>(this)->CalculateLayout(AllottedGeometry);
    
    // 先绘制边
    for (const FMAPreviewEdgeRenderData& Edge : EdgeRenderData)
    {
        DrawEdge(Edge, AllottedGeometry, OutDrawElements, LayerId);
    }
    
    // 再绘制节点
    for (int32 i = 0; i < NodeRenderData.Num(); ++i)
    {
        DrawNode(NodeRenderData[i], i, AllottedGeometry, OutDrawElements, LayerId + 1);
    }
}

void UMATaskGraphPreview::DrawNode(const FMAPreviewNodeRenderData& Node, int32 NodeIndex, 
                                    const FGeometry& AllottedGeometry, 
                                    FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 获取状态颜色
    FLinearColor NodeColor = GetStatusColor(Node.Status);
    
    // 圆角半径
    const float CornerRadius = 4.0f;
    
    // 创建圆角矩形画刷
    FSlateRoundedBoxBrush RoundedBrush(NodeColor, CornerRadius);
    
    // 绘制节点背景 (圆角矩形)
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(Node.Size, FSlateLayoutTransform(Node.Position)),
        &RoundedBrush,
        ESlateDrawEffect::None,
        NodeColor
    );
    
    // 如果是悬浮节点，绘制高亮边框
    if (NodeIndex == HoveredNodeIndex)
    {
        // 绘制高亮叠加层 (圆角矩形)
        FSlateRoundedBoxBrush HighlightBrush(HoverHighlightColor, CornerRadius);
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(Node.Size, FSlateLayoutTransform(Node.Position)),
            &HighlightBrush,
            ESlateDrawEffect::None,
            HoverHighlightColor
        );
    }
    
    // 绘制节点文本 (显示 TaskId) - 居中显示
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Bold", 7);
    
    // 截断显示文本
    FString DisplayText = Node.NodeId;
    if (DisplayText.Len() > 8)
    {
        DisplayText = DisplayText.Left(6) + TEXT("..");
    }
    
    // 计算文本居中位置
    float EstimatedTextWidth = DisplayText.Len() * 5.0f;  // 估算字符宽度
    float EstimatedTextHeight = 9.0f;  // 估算字体高度
    float CenteredX = Node.Position.X + (Node.Size.X - EstimatedTextWidth) / 2.0f;
    float CenteredY = Node.Position.Y + (Node.Size.Y - EstimatedTextHeight) / 2.0f;
    FVector2D TextPos(CenteredX, CenteredY);
    
    // 使用深色文字以便在浅色背景上可见
    // 从 Theme 获取 InputTextColor，或使用 fallback 默认值
    FLinearColor TextOnNodeColor = Theme ? Theme->InputTextColor : FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    FSlateDrawElement::MakeText(
        OutDrawElements,
        LayerId + 2,
        AllottedGeometry.ToPaintGeometry(FVector2D(Node.Size.X - 4, Node.Size.Y - 4), FSlateLayoutTransform(TextPos)),
        FText::FromString(DisplayText),
        FontInfo,
        ESlateDrawEffect::None,
        TextOnNodeColor
    );
}

void UMATaskGraphPreview::DrawEdge(const FMAPreviewEdgeRenderData& Edge, const FGeometry& AllottedGeometry, 
                                    FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    DrawArrow(Edge.StartPoint, Edge.EndPoint, EdgeColor, AllottedGeometry, OutDrawElements, LayerId);
}

void UMATaskGraphPreview::DrawArrow(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color,
                                     const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    // 绘制线段
    TArray<FVector2D> Points;
    Points.Add(Start);
    Points.Add(End);
    
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        Points,
        ESlateDrawEffect::None,
        Color,
        true,
        1.5f
    );
    
    // 绘制箭头
    FVector2D Direction = (End - Start).GetSafeNormal();
    FVector2D Perpendicular(-Direction.Y, Direction.X);
    float ArrowSize = 6.0f;
    
    FVector2D ArrowTip = End;
    FVector2D ArrowLeft = End - Direction * ArrowSize + Perpendicular * ArrowSize * 0.5f;
    FVector2D ArrowRight = End - Direction * ArrowSize - Perpendicular * ArrowSize * 0.5f;
    
    TArray<FVector2D> ArrowPoints;
    ArrowPoints.Add(ArrowLeft);
    ArrowPoints.Add(ArrowTip);
    ArrowPoints.Add(ArrowRight);
    
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        ArrowPoints,
        ESlateDrawEffect::None,
        Color,
        true,
        1.5f
    );
}


void UMATaskGraphPreview::DrawTooltip(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    if (HoveredNodeIndex < 0 || HoveredNodeIndex >= NodeRenderData.Num())
    {
        return;
    }
    
    const FMAPreviewNodeRenderData& Node = NodeRenderData[HoveredNodeIndex];
    
    // 构建工具提示文本
    FString StatusStr;
    switch (Node.Status)
    {
    case EMATaskExecutionStatus::Pending:
        StatusStr = TEXT("待执行");
        break;
    case EMATaskExecutionStatus::InProgress:
        StatusStr = TEXT("执行中");
        break;
    case EMATaskExecutionStatus::Completed:
        StatusStr = TEXT("已完成");
        break;
    case EMATaskExecutionStatus::Failed:
        StatusStr = TEXT("失败");
        break;
    }
    
    TArray<FString> TooltipLines;
    TooltipLines.Add(FString::Printf(TEXT("ID: %s"), *Node.NodeId));
    if (!Node.Description.IsEmpty())
    {
        TooltipLines.Add(FString::Printf(TEXT("描述: %s"), *Node.Description));
    }
    if (!Node.Location.IsEmpty())
    {
        TooltipLines.Add(FString::Printf(TEXT("位置: %s"), *Node.Location));
    }
    TooltipLines.Add(FString::Printf(TEXT("状态: %s"), *StatusStr));
    
    // 计算工具提示大小
    float TooltipWidth = 150.0f;
    float LineHeight = 14.0f;
    float TooltipHeight = TooltipLines.Num() * LineHeight + 10.0f;
    float TooltipPadding = 5.0f;
    
    // 计算工具提示位置 (在节点右上方)
    FVector2D TooltipPos = Node.Position + FVector2D(NodeWidth + 5, -TooltipHeight / 2);
    
    // 确保工具提示不超出边界
    FVector2D CanvasSize = AllottedGeometry.GetLocalSize();
    if (TooltipPos.X + TooltipWidth > CanvasSize.X - 5)
    {
        TooltipPos.X = Node.Position.X - TooltipWidth - 5;
    }
    if (TooltipPos.Y < 5)
    {
        TooltipPos.Y = 5;
    }
    if (TooltipPos.Y + TooltipHeight > CanvasSize.Y - 5)
    {
        TooltipPos.Y = CanvasSize.Y - TooltipHeight - 5;
    }
    
    // 绘制工具提示背景
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(FVector2D(TooltipWidth, TooltipHeight), FSlateLayoutTransform(TooltipPos)),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        TooltipBgColor
    );
    
    // 绘制工具提示边框
    TArray<FVector2D> BorderPoints;
    BorderPoints.Add(TooltipPos);
    BorderPoints.Add(TooltipPos + FVector2D(TooltipWidth, 0));
    BorderPoints.Add(TooltipPos + FVector2D(TooltipWidth, TooltipHeight));
    BorderPoints.Add(TooltipPos + FVector2D(0, TooltipHeight));
    BorderPoints.Add(TooltipPos);
    
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId + 1,
        AllottedGeometry.ToPaintGeometry(),
        BorderPoints,
        ESlateDrawEffect::None,
        FLinearColor(0.3f, 0.3f, 0.35f, 1.0f),
        true,
        1.0f
    );
    
    // 绘制工具提示文本
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 9);
    
    for (int32 i = 0; i < TooltipLines.Num(); ++i)
    {
        FVector2D TextPos = TooltipPos + FVector2D(TooltipPadding, TooltipPadding + i * LineHeight);
        
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(FVector2D(TooltipWidth - TooltipPadding * 2, LineHeight), FSlateLayoutTransform(TextPos)),
            FText::FromString(TooltipLines[i]),
            FontInfo,
            ESlateDrawEffect::None,
            TooltipTextColor
        );
    }
}

//=============================================================================
// 辅助方法
//=============================================================================

FLinearColor UMATaskGraphPreview::GetStatusColor(EMATaskExecutionStatus Status) const
{
    switch (Status)
    {
    case EMATaskExecutionStatus::Pending:
        return PendingColor;      // 灰色
    case EMATaskExecutionStatus::InProgress:
        return InProgressColor;   // 黄色
    case EMATaskExecutionStatus::Completed:
        return CompletedColor;    // 绿色
    case EMATaskExecutionStatus::Failed:
        return FailedColor;       // 红色
    default:
        return PendingColor;
    }
}

int32 UMATaskGraphPreview::FindNodeAtPosition(const FVector2D& LocalPosition) const
{
    for (int32 i = 0; i < NodeRenderData.Num(); ++i)
    {
        const FMAPreviewNodeRenderData& Node = NodeRenderData[i];
        
        // 检查点是否在节点矩形内
        if (LocalPosition.X >= Node.Position.X && 
            LocalPosition.X <= Node.Position.X + Node.Size.X &&
            LocalPosition.Y >= Node.Position.Y && 
            LocalPosition.Y <= Node.Position.Y + Node.Size.Y)
        {
            return i;
        }
    }
    
    return -1;  // 未找到
}

//=============================================================================
// 主题应用
//=============================================================================

void UMATaskGraphPreview::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    
    if (!Theme)
    {
        UE_LOG(LogMATaskGraphPreview, Warning, TEXT("ApplyTheme: Theme is null, using fallback defaults"));
        return;
    }
    
    // 应用背景颜色 - 使用画布背景色保持与 SkillListPreview 一致
    BackgroundColor = Theme->CanvasBackgroundColor;
    
    // 文本颜色
    TextColor = Theme->SecondaryTextColor;
    HintTextColor = Theme->HintTextColor;
    TooltipTextColor = Theme->TextColor;
    
    // 状态颜色
    PendingColor = Theme->StatusPendingColor;
    InProgressColor = Theme->StatusInProgressColor;
    CompletedColor = Theme->StatusCompletedColor;
    FailedColor = Theme->StatusFailedColor;
    
    // 画布颜色
    EdgeColor = Theme->EdgeColor;
    
    // 交互颜色
    TooltipBgColor = Theme->HighlightColor;
    HoverHighlightColor = Theme->HighlightColor;
    HoverHighlightColor.A = 0.3f;
    
    // 更新内容边框背景
    if (ContentBorder)
    {
        // 应用圆角效果
        float CornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(Theme, EMARoundedElementType::Button);
        MARoundedBorderUtils::ApplyRoundedCorners(ContentBorder, BackgroundColor, CornerRadius);
    }
    
    // 更新统计文本颜色
    if (StatsText)
    {
        StatsText->SetColorAndOpacity(FSlateColor(TextColor));
    }
    
    // 更新空状态提示文本颜色
    if (EmptyText)
    {
        EmptyText->SetColorAndOpacity(FSlateColor(HintTextColor));
    }
    
    UE_LOG(LogMATaskGraphPreview, Log, TEXT("ApplyTheme: Theme applied successfully"));
}
