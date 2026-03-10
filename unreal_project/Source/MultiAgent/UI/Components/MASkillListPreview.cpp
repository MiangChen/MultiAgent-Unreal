#include "MASkillListPreview.h"

#include "UI/Components/Application/MASkillListPreviewCoordinator.h"
#include "UI/Components/Infrastructure/MASkillListPreviewLayout.h"
#include "../Core/MAUITheme.h"
#include "../Core/MARoundedBorderUtils.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASkillListPreview, Log, All);

UMASkillListPreview::UMASkillListPreview(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMASkillListPreview::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UMASkillListPreview::NativeConstruct()
{
    Super::NativeConstruct();
}

TSharedRef<SWidget> UMASkillListPreview::RebuildWidget()
{
    BuildUI();
    return Super::RebuildWidget();
}

int32 UMASkillListPreview::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                       const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                       int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    if (bHasData && SkillBarRenderData.Num() > 0)
    {
        DrawGantt(AllottedGeometry, OutDrawElements, LayerId);
        if (HoveredBarIndex >= 0 && HoveredBarIndex < SkillBarRenderData.Num())
        {
            DrawTooltip(AllottedGeometry, OutDrawElements, LayerId + 10);
        }
    }

    return LayerId;
}

void UMASkillListPreview::BuildUI()
{
    if (!WidgetTree)
    {
        UE_LOG(LogMASkillListPreview, Error, TEXT("BuildUI: WidgetTree is null"));
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
        UE_LOG(LogMASkillListPreview, Error, TEXT("BuildUI: Failed to create RootSizeBox"));
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
        EmptyText->SetText(FText::FromString(TEXT("Waiting for skill list...")));
        EmptyText->SetColorAndOpacity(FSlateColor(TextColor));
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

void UMASkillListPreview::UpdatePreview(const FMASkillAllocationData& Data)
{
    CurrentModel = FMASkillListPreviewCoordinator::BuildModel(Data);
    bHasData = CurrentModel.bHasData;
    HoveredBarIndex = -1;
    SkillBarRenderData.Empty();
    ApplyModelState();

    UE_LOG(LogMASkillListPreview, Log, TEXT("UpdatePreview: %d robots, %d time steps, %d skill bars"),
        CurrentModel.RobotIds.Num(), CurrentModel.TimeSteps.Num(), CurrentModel.Bars.Num());
}

void UMASkillListPreview::UpdateSkillStatus(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus)
{
    if (!bHasData)
    {
        return;
    }

    if (FMASkillListPreviewCoordinator::UpdateSkillStatus(CurrentModel, TimeStep, RobotId, NewStatus))
    {
        for (FMASkillListPreviewBarRenderData& Bar : SkillBarRenderData)
        {
            if (Bar.TimeStep == TimeStep && Bar.RobotId == RobotId)
            {
                Bar.Status = NewStatus;
                break;
            }
        }
        Invalidate(EInvalidateWidgetReason::Paint);
    }
}

void UMASkillListPreview::ClearPreview()
{
    CurrentModel = FMASkillListPreviewCoordinator::MakeEmptyModel();
    bHasData = false;
    HoveredBarIndex = -1;
    SkillBarRenderData.Empty();
    ApplyModelState();
}

void UMASkillListPreview::RefreshRenderData(const FGeometry& AllottedGeometry)
{
    const FMASkillListPreviewLayoutConfig Config{TopMargin, LeftMargin, RightMargin, BottomMargin, RowHeight, BarPadding};
    const FMASkillListPreviewLayoutResult Layout = FMASkillListPreviewLayout::BuildLayout(CurrentModel, AllottedGeometry.GetLocalSize(), Config);
    SkillBarRenderData = Layout.Bars;
}

void UMASkillListPreview::ApplyModelState()
{
    if (StatsText)
    {
        StatsText->SetText(FText::FromString(CurrentModel.StatsText));
        StatsText->SetColorAndOpacity(FSlateColor(TextColor));
    }

    if (EmptyText)
    {
        EmptyText->SetVisibility(bHasData ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
        EmptyText->SetColorAndOpacity(FSlateColor(Theme ? Theme->HintTextColor : TextColor));
    }

    Invalidate(EInvalidateWidgetReason::Paint);
}

void UMASkillListPreview::DrawGantt(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    const_cast<UMASkillListPreview*>(this)->RefreshRenderData(AllottedGeometry);
    DrawGrid(AllottedGeometry, OutDrawElements, LayerId);
    DrawRobotLabels(AllottedGeometry, OutDrawElements, LayerId + 1);
    DrawSkillBars(AllottedGeometry, OutDrawElements, LayerId + 2);
}

void UMASkillListPreview::DrawGrid(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    const FVector2D CanvasSize = AllottedGeometry.GetLocalSize();
    const float AvailableWidth = CanvasSize.X - LeftMargin - RightMargin;
    const float AvailableHeight = CanvasSize.Y - TopMargin - BottomMargin;
    const float ActualRowHeight = CurrentModel.RobotIds.Num() > 0
        ? FMath::Min(RowHeight, AvailableHeight / static_cast<float>(CurrentModel.RobotIds.Num()))
        : RowHeight;
    const float TimeStepWidth = CurrentModel.TimeSteps.Num() > 0
        ? AvailableWidth / static_cast<float>(CurrentModel.TimeSteps.Num())
        : AvailableWidth;

    for (int32 Index = 0; Index <= CurrentModel.TimeSteps.Num(); ++Index)
    {
        const float X = LeftMargin + Index * TimeStepWidth;
        TArray<FVector2D> LinePoints;
        LinePoints.Add(FVector2D(X, TopMargin));
        LinePoints.Add(FVector2D(X, TopMargin + CurrentModel.RobotIds.Num() * ActualRowHeight));
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            LinePoints,
            ESlateDrawEffect::None,
            GridLineColor,
            true,
            1.0f);
    }
}

void UMASkillListPreview::DrawRobotLabels(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    const FVector2D CanvasSize = AllottedGeometry.GetLocalSize();
    const float AvailableHeight = CanvasSize.Y - TopMargin - BottomMargin;
    const float ActualRowHeight = CurrentModel.RobotIds.Num() > 0
        ? FMath::Min(RowHeight, AvailableHeight / static_cast<float>(CurrentModel.RobotIds.Num()))
        : RowHeight;

    for (int32 Index = 0; Index < CurrentModel.RobotIds.Num(); ++Index)
    {
        FString DisplayId = CurrentModel.RobotIds[Index];
        if (DisplayId.Len() > 8)
        {
            DisplayId = DisplayId.Left(6) + TEXT("..");
        }

        const float Y = TopMargin + Index * ActualRowHeight + ActualRowHeight / 2.0f - 6.0f;
        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2D(LeftMargin - 5.0f, ActualRowHeight), FSlateLayoutTransform(FVector2D(5.0f, Y))),
            FText::FromString(DisplayId),
            FCoreStyle::GetDefaultFontStyle("Regular", 8),
            ESlateDrawEffect::None,
            TextColor);
    }
}

void UMASkillListPreview::DrawSkillBars(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    const float CornerRadius = 6.0f;
    for (int32 Index = 0; Index < SkillBarRenderData.Num(); ++Index)
    {
        const FMASkillListPreviewBarRenderData& Bar = SkillBarRenderData[Index];
        const FLinearColor BarColor = GetStatusColor(Bar.Status);
        FSlateRoundedBoxBrush RoundedBrush(BarColor, CornerRadius);
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(Bar.Size, FSlateLayoutTransform(Bar.Position)),
            &RoundedBrush,
            ESlateDrawEffect::None,
            BarColor);

        if (Index == HoveredBarIndex)
        {
            FSlateRoundedBoxBrush HighlightBrush(HoverHighlightColor, CornerRadius);
            FSlateDrawElement::MakeBox(
                OutDrawElements,
                LayerId + 1,
                AllottedGeometry.ToPaintGeometry(Bar.Size, FSlateLayoutTransform(Bar.Position)),
                &HighlightBrush,
                ESlateDrawEffect::None,
                HoverHighlightColor);
        }

        if (Bar.Size.X > 30.0f && Bar.Size.Y > 10.0f)
        {
            FString DisplayName = Bar.SkillName;
            if (DisplayName.Len() > 8)
            {
                DisplayName = DisplayName.Left(6) + TEXT("..");
            }

            const float EstimatedTextWidth = DisplayName.Len() * 4.5f;
            const float EstimatedTextHeight = 9.0f;
            const FVector2D TextPos(
                FMath::Max(Bar.Position.X + (Bar.Size.X - EstimatedTextWidth) / 2.0f, Bar.Position.X + 2.0f),
                FMath::Max(Bar.Position.Y + (Bar.Size.Y - EstimatedTextHeight) / 2.0f, Bar.Position.Y + 1.0f));
            const FLinearColor TextOnBarColor = Theme ? Theme->InputTextColor : FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId + 2,
                AllottedGeometry.ToPaintGeometry(Bar.Size - FVector2D(4.0f, 2.0f), FSlateLayoutTransform(TextPos)),
                FText::FromString(DisplayName),
                FCoreStyle::GetDefaultFontStyle("Regular", 7),
                ESlateDrawEffect::None,
                TextOnBarColor);
        }
    }
}

FLinearColor UMASkillListPreview::GetStatusColor(ESkillExecutionStatus Status) const
{
    switch (Status)
    {
    case ESkillExecutionStatus::Pending:
        return PendingColor;
    case ESkillExecutionStatus::InProgress:
        return InProgressColor;
    case ESkillExecutionStatus::Completed:
        return CompletedColor;
    case ESkillExecutionStatus::Failed:
        return FailedColor;
    default:
        return PendingColor;
    }
}

int32 UMASkillListPreview::FindSkillBarAtPosition(const FVector2D& LocalPosition) const
{
    for (int32 Index = 0; Index < SkillBarRenderData.Num(); ++Index)
    {
        const FMASkillListPreviewBarRenderData& Bar = SkillBarRenderData[Index];
        if (LocalPosition.X >= Bar.Position.X && LocalPosition.X <= Bar.Position.X + Bar.Size.X
            && LocalPosition.Y >= Bar.Position.Y && LocalPosition.Y <= Bar.Position.Y + Bar.Size.Y)
        {
            return Index;
        }
    }
    return INDEX_NONE;
}

FReply UMASkillListPreview::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    const int32 NewHoveredIndex = FindSkillBarAtPosition(LocalPos);
    if (NewHoveredIndex != HoveredBarIndex)
    {
        HoveredBarIndex = NewHoveredIndex;
        Invalidate(EInvalidateWidgetReason::Paint);
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UMASkillListPreview::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    if (HoveredBarIndex >= 0)
    {
        HoveredBarIndex = -1;
        Invalidate(EInvalidateWidgetReason::Paint);
    }

    Super::NativeOnMouseLeave(InMouseEvent);
}

void UMASkillListPreview::DrawTooltip(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    if (HoveredBarIndex < 0 || HoveredBarIndex >= SkillBarRenderData.Num())
    {
        return;
    }

    const FMASkillListPreviewBarRenderData& Bar = SkillBarRenderData[HoveredBarIndex];
    FString StatusStr = TEXT("Pending");
    switch (Bar.Status)
    {
    case ESkillExecutionStatus::Pending:
        StatusStr = TEXT("Pending");
        break;
    case ESkillExecutionStatus::InProgress:
        StatusStr = TEXT("In Progress");
        break;
    case ESkillExecutionStatus::Completed:
        StatusStr = TEXT("Completed");
        break;
    case ESkillExecutionStatus::Failed:
        StatusStr = TEXT("Failed");
        break;
    }

    const TArray<FString> TooltipLines{
        FString::Printf(TEXT("Robot: %s"), *Bar.RobotId),
        FString::Printf(TEXT("Skill: %s"), *Bar.SkillName),
        FString::Printf(TEXT("Time Step: %d"), Bar.TimeStep),
        FString::Printf(TEXT("Status: %s"), *StatusStr)};

    const float TooltipWidth = 150.0f;
    const float LineHeight = 14.0f;
    const float TooltipHeight = TooltipLines.Num() * LineHeight + 10.0f;
    const float TooltipPadding = 5.0f;
    FVector2D TooltipPos = Bar.Position + FVector2D(0.0f, -TooltipHeight - 5.0f);

    const FVector2D CanvasSize = AllottedGeometry.GetLocalSize();
    TooltipPos.X = FMath::Clamp(TooltipPos.X, 5.0f, CanvasSize.X - TooltipWidth - 5.0f);
    if (TooltipPos.Y < 5.0f)
    {
        TooltipPos.Y = Bar.Position.Y + Bar.Size.Y + 5.0f;
    }

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

    FLinearColor TooltipBorderColor = GridLineColor;
    TooltipBorderColor.A = 1.0f;
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId + 1,
        AllottedGeometry.ToPaintGeometry(),
        BorderPoints,
        ESlateDrawEffect::None,
        TooltipBorderColor,
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

void UMASkillListPreview::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    if (!Theme)
    {
        UE_LOG(LogMASkillListPreview, Warning, TEXT("ApplyTheme: Theme is null, using fallback defaults"));
        return;
    }

    BackgroundColor = Theme->CanvasBackgroundColor;
    GridLineColor = Theme->GridLineColor;
    GridLineColor.A = 0.5f;
    TextColor = Theme->SecondaryTextColor;
    TooltipTextColor = Theme->TextColor;
    PendingColor = Theme->StatusPendingColor;
    InProgressColor = Theme->StatusInProgressColor;
    CompletedColor = Theme->StatusCompletedColor;
    FailedColor = Theme->StatusFailedColor;
    TooltipBgColor = Theme->HighlightColor;
    HoverHighlightColor = Theme->HighlightColor;
    HoverHighlightColor.A = 0.3f;

    if (ContentBorder)
    {
        const float CornerRadius = MARoundedBorderUtils::GetCornerRadiusForType(Theme, EMARoundedElementType::Button);
        MARoundedBorderUtils::ApplyRoundedCorners(ContentBorder, BackgroundColor, CornerRadius);
    }

    ApplyModelState();
    UE_LOG(LogMASkillListPreview, Log, TEXT("ApplyTheme: Theme applied successfully"));
}
