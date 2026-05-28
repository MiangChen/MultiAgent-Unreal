#include "MATaskGraphPreview.h"

#include "UI/Components/Application/MATaskGraphPreviewCoordinator.h"
#include "UI/Components/Infrastructure/MATaskGraphPreviewLayout.h"
#include "../../Core/MAUITheme.h"
#include "../../Core/MARoundedBorderUtils.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogMATaskGraphPreview, Log, All);

UMATaskGraphPreview::UMATaskGraphPreview(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

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
    LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    if (bHasData && CurrentModel.Nodes.Num() > 0)
    {
        DrawDAG(AllottedGeometry, OutDrawElements, LayerId);
        if (HoveredNodeIndex >= 0 && HoveredNodeIndex < NodeRenderData.Num())
        {
            DrawTooltip(AllottedGeometry, OutDrawElements, LayerId + 10);
        }
    }

    return LayerId;
}

FReply UMATaskGraphPreview::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    const int32 NewHoveredIndex = FindNodeAtPosition(LocalPos);
    if (NewHoveredIndex != HoveredNodeIndex)
    {
        HoveredNodeIndex = NewHoveredIndex;
        Invalidate(EInvalidateWidgetReason::Paint);
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UMATaskGraphPreview::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    if (HoveredNodeIndex >= 0)
    {
        HoveredNodeIndex = -1;
        Invalidate(EInvalidateWidgetReason::Paint);
    }

    Super::NativeOnMouseLeave(InMouseEvent);
}

void UMATaskGraphPreview::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMATaskGraphPreview, Error, TEXT("BuildUI: WidgetTree is null"));
        return;
    }

    if (!Theme)
    {
        Theme = NewObject<UMAUITheme>();
    }
    BackgroundColor = Theme->CanvasBackgroundColor;

    RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
    if (!RootSizeBox)
    {
        UE_LOG(LogMATaskGraphPreview, Error, TEXT("BuildUI: Failed to create RootSizeBox"));
        return;
    }
    RootSizeBox->SetMinDesiredHeight(PreviewHeight);
    WidgetTree->RootWidget = RootSizeBox;

    ContentBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ContentBorder"));
    if (!ContentBorder)
    {
        return;
    }
    ContentBorder->SetPadding(FMargin(8.0f));
    MARoundedBorderUtils::ApplyRoundedCorners(ContentBorder, BackgroundColor, MARoundedBorderUtils::DefaultButtonCornerRadius);

    UVerticalBox* ContentVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentVBox"));
    if (!ContentVBox)
    {
        return;
    }

    StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatsText"));
    if (StatsText)
    {
        StatsText->SetColorAndOpacity(FSlateColor(TextColor));
        StatsText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
        if (UVerticalBoxSlot* StatsSlot = ContentVBox->AddChildToVerticalBox(StatsText))
        {
            StatsSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
        }
    }

    EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
    if (EmptyText)
    {
        EmptyText->SetText(FText::FromString(TEXT("Waiting for task graph...")));
        EmptyText->SetColorAndOpacity(FSlateColor(HintTextColor));
        EmptyText->SetJustification(ETextJustify::Center);
        EmptyText->SetFont(FCoreStyle::GetDefaultFontStyle("Italic", 10));
        if (UVerticalBoxSlot* EmptySlot = ContentVBox->AddChildToVerticalBox(EmptyText))
        {
            EmptySlot->SetHorizontalAlignment(HAlign_Center);
            EmptySlot->SetVerticalAlignment(VAlign_Center);
            EmptySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    ContentBorder->SetContent(ContentVBox);
    RootSizeBox->SetContent(ContentBorder);
    ApplyModelState();
}

void UMATaskGraphPreview::UpdatePreview(const FMATaskGraphData& Data)
{
    CurrentModel = FMATaskGraphPreviewCoordinator::BuildModel(Data);
    bHasData = CurrentModel.bHasData;
    HoveredNodeIndex = -1;
    NodeRenderData.Empty();
    EdgeRenderData.Empty();
    ApplyModelState();

    UE_LOG(LogMATaskGraphPreview, Log, TEXT("UpdatePreview: %d nodes, %d edges"), CurrentModel.Nodes.Num(), CurrentModel.Edges.Num());
}

void UMATaskGraphPreview::ClearPreview()
{
    CurrentModel = FMATaskGraphPreviewCoordinator::MakeEmptyModel();
    bHasData = false;
    HoveredNodeIndex = -1;
    NodeRenderData.Empty();
    EdgeRenderData.Empty();
    ApplyModelState();
}

void UMATaskGraphPreview::RefreshRenderData(const FGeometry& AllottedGeometry)
{
    const FMATaskGraphPreviewLayoutConfig Config{NodeWidth, NodeHeight, TopMargin, LeftMargin};
    const FMATaskGraphPreviewLayoutResult Layout = FMATaskGraphPreviewLayout::BuildLayout(CurrentModel, AllottedGeometry.GetLocalSize(), Config);
    NodeRenderData = Layout.Nodes;
    EdgeRenderData = Layout.Edges;
}

void UMATaskGraphPreview::ApplyModelState()
{
    if (StatsText)
    {
        StatsText->SetText(FText::FromString(CurrentModel.StatsText));
        StatsText->SetColorAndOpacity(FSlateColor(TextColor));
    }

    if (EmptyText)
    {
        EmptyText->SetVisibility(bHasData ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
        EmptyText->SetColorAndOpacity(FSlateColor(HintTextColor));
    }

    Invalidate(EInvalidateWidgetReason::Paint);
}

void UMATaskGraphPreview::DrawDAG(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    const_cast<UMATaskGraphPreview*>(this)->RefreshRenderData(AllottedGeometry);

    for (const FMATaskGraphPreviewEdgeRenderData& Edge : EdgeRenderData)
    {
        DrawEdge(Edge, AllottedGeometry, OutDrawElements, LayerId);
    }

    for (int32 Index = 0; Index < NodeRenderData.Num(); ++Index)
    {
        DrawNode(NodeRenderData[Index], Index, AllottedGeometry, OutDrawElements, LayerId + 1);
    }
}

void UMATaskGraphPreview::DrawNode(const FMATaskGraphPreviewNodeRenderData& Node, int32 NodeIndex,
                                   const FGeometry& AllottedGeometry,
                                   FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    const FLinearColor NodeColor = GetStatusColor(Node.Status);
    const float CornerRadius = 4.0f;
    FSlateRoundedBoxBrush RoundedBrush(NodeColor, CornerRadius);

    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(Node.Size, FSlateLayoutTransform(Node.Position)),
        &RoundedBrush,
        ESlateDrawEffect::None,
        NodeColor);

    if (NodeIndex == HoveredNodeIndex)
    {
        FSlateRoundedBoxBrush HighlightBrush(HoverHighlightColor, CornerRadius);
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(Node.Size, FSlateLayoutTransform(Node.Position)),
            &HighlightBrush,
            ESlateDrawEffect::None,
            HoverHighlightColor);
    }

    FString DisplayText = Node.NodeId;
    if (DisplayText.Len() > 8)
    {
        DisplayText = DisplayText.Left(6) + TEXT("..");
    }

    const float EstimatedTextWidth = DisplayText.Len() * 5.0f;
    const float EstimatedTextHeight = 9.0f;
    const FVector2D TextPos(
        Node.Position.X + (Node.Size.X - EstimatedTextWidth) / 2.0f,
        Node.Position.Y + (Node.Size.Y - EstimatedTextHeight) / 2.0f);

    const FLinearColor TextOnNodeColor = Theme ? Theme->InputTextColor : FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
    FSlateDrawElement::MakeText(
        OutDrawElements,
        LayerId + 2,
        AllottedGeometry.ToPaintGeometry(FVector2D(Node.Size.X - 4.0f, Node.Size.Y - 4.0f), FSlateLayoutTransform(TextPos)),
        FText::FromString(DisplayText),
        FCoreStyle::GetDefaultFontStyle("Bold", 7),
        ESlateDrawEffect::None,
        TextOnNodeColor);
}

void UMATaskGraphPreview::DrawEdge(const FMATaskGraphPreviewEdgeRenderData& Edge, const FGeometry& AllottedGeometry,
                                   FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    DrawArrow(Edge.StartPoint, Edge.EndPoint, EdgeColor, AllottedGeometry, OutDrawElements, LayerId);
}

void UMATaskGraphPreview::DrawArrow(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color,
                                    const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    TArray<FVector2D> Points{Start, End};
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        Points,
        ESlateDrawEffect::None,
        Color,
        true,
        1.5f);

    const FVector2D Direction = (End - Start).GetSafeNormal();
    const FVector2D Perpendicular(-Direction.Y, Direction.X);
    const float ArrowSize = 6.0f;
    const FVector2D ArrowTip = End;
    const FVector2D ArrowLeft = End - Direction * ArrowSize + Perpendicular * ArrowSize * 0.5f;
    const FVector2D ArrowRight = End - Direction * ArrowSize - Perpendicular * ArrowSize * 0.5f;

    TArray<FVector2D> ArrowPoints{ArrowLeft, ArrowTip, ArrowRight};
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        ArrowPoints,
        ESlateDrawEffect::None,
        Color,
        true,
        1.5f);
}

void UMATaskGraphPreview::DrawTooltip(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    if (HoveredNodeIndex < 0 || HoveredNodeIndex >= NodeRenderData.Num())
    {
        return;
    }

    const FMATaskGraphPreviewNodeRenderData& Node = NodeRenderData[HoveredNodeIndex];
    FString StatusStr = TEXT("Pending");
    switch (Node.Status)
    {
    case EMATaskExecutionStatus::Pending:
        StatusStr = TEXT("Pending");
        break;
    case EMATaskExecutionStatus::InProgress:
        StatusStr = TEXT("In Progress");
        break;
    case EMATaskExecutionStatus::Completed:
        StatusStr = TEXT("Completed");
        break;
    case EMATaskExecutionStatus::Failed:
        StatusStr = TEXT("Failed");
        break;
    }

    TArray<FString> TooltipLines;
    TooltipLines.Add(FString::Printf(TEXT("ID: %s"), *Node.NodeId));
    if (!Node.Description.IsEmpty())
    {
        TooltipLines.Add(FString::Printf(TEXT("Description: %s"), *Node.Description));
    }
    if (!Node.Location.IsEmpty())
    {
        TooltipLines.Add(FString::Printf(TEXT("Location: %s"), *Node.Location));
    }
    TooltipLines.Add(FString::Printf(TEXT("Status: %s"), *StatusStr));

    const float TooltipWidth = 180.0f;
    const float LineHeight = 14.0f;
    const float TooltipHeight = TooltipLines.Num() * LineHeight + 10.0f;
    const float TooltipPadding = 5.0f;
    FVector2D TooltipPos = Node.Position + FVector2D(NodeWidth + 5.0f, -TooltipHeight / 2.0f);

    const FVector2D CanvasSize = AllottedGeometry.GetLocalSize();
    if (TooltipPos.X + TooltipWidth > CanvasSize.X - 5.0f)
    {
        TooltipPos.X = Node.Position.X - TooltipWidth - 5.0f;
    }
    TooltipPos.Y = FMath::Clamp(TooltipPos.Y, 5.0f, CanvasSize.Y - TooltipHeight - 5.0f);

    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(FVector2D(TooltipWidth, TooltipHeight), FSlateLayoutTransform(TooltipPos)),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        TooltipBgColor);

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
        1.0f);

    for (int32 Index = 0; Index < TooltipLines.Num(); ++Index)
    {
        const FVector2D TextPos = TooltipPos + FVector2D(TooltipPadding, TooltipPadding + Index * LineHeight);
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(FVector2D(TooltipWidth - TooltipPadding * 2.0f, LineHeight), FSlateLayoutTransform(TextPos)),
            FText::FromString(TooltipLines[Index]),
            FCoreStyle::GetDefaultFontStyle("Regular", 9),
            ESlateDrawEffect::None,
            TooltipTextColor);
    }
}

FLinearColor UMATaskGraphPreview::GetStatusColor(EMATaskExecutionStatus Status) const
{
    switch (Status)
    {
    case EMATaskExecutionStatus::Pending:
        return PendingColor;
    case EMATaskExecutionStatus::InProgress:
        return InProgressColor;
    case EMATaskExecutionStatus::Completed:
        return CompletedColor;
    case EMATaskExecutionStatus::Failed:
        return FailedColor;
    default:
        return PendingColor;
    }
}

int32 UMATaskGraphPreview::FindNodeAtPosition(const FVector2D& LocalPosition) const
{
    for (int32 Index = 0; Index < NodeRenderData.Num(); ++Index)
    {
        const FMATaskGraphPreviewNodeRenderData& Node = NodeRenderData[Index];
        if (LocalPosition.X >= Node.Position.X && LocalPosition.X <= Node.Position.X + Node.Size.X
            && LocalPosition.Y >= Node.Position.Y && LocalPosition.Y <= Node.Position.Y + Node.Size.Y)
        {
            return Index;
        }
    }
    return INDEX_NONE;
}

void UMATaskGraphPreview::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme)
    {
        UE_LOG(LogMATaskGraphPreview, Warning, TEXT("ApplyTheme: Theme is null, using fallback defaults"));
        return;
    }

    BackgroundColor = Theme->CanvasBackgroundColor;
    TextColor = Theme->SecondaryTextColor;
    HintTextColor = Theme->HintTextColor;
    TooltipTextColor = Theme->TextColor;
    PendingColor = Theme->StatusPendingColor;
    InProgressColor = Theme->StatusInProgressColor;
    CompletedColor = Theme->StatusCompletedColor;
    FailedColor = Theme->StatusFailedColor;
    EdgeColor = Theme->EdgeColor;
    TooltipBgColor = Theme->HighlightColor;
    HoverHighlightColor = Theme->HighlightColor;
    HoverHighlightColor.A = 0.3f;

    if (ContentBorder)
    {
        const float CornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(Theme, EMARoundedElementType::Button);
        MARoundedBorderUtils::ApplyRoundedCorners(ContentBorder, BackgroundColor, CornerRadius);
    }

    ApplyModelState();
    UE_LOG(LogMATaskGraphPreview, Log, TEXT("ApplyTheme: Theme applied successfully"));
}
