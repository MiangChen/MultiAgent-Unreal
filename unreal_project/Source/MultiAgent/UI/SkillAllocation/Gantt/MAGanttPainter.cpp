// MAGanttPainter.cpp

#include "MAGanttPainter.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

float FMAGanttPainter::TimeStepToScreen(const FMAGanttPainterContext& Ctx, int32 TimeStep)
{
    return Ctx.GridLayout.TimeStepToScreen(TimeStep, Ctx.LabelWidth, Ctx.TimeStepWidth, Ctx.ScrollOffset.X);
}

float FMAGanttPainter::RobotIndexToScreen(const FMAGanttPainterContext& Ctx, int32 RobotIndex)
{
    return Ctx.GridLayout.RobotIndexToScreen(RobotIndex, Ctx.HeaderHeight, Ctx.RobotRowHeight, Ctx.ScrollOffset.Y);
}

void FMAGanttPainter::DrawTimelineHeader(const FMAGanttPainterContext& Ctx, int32 LayerId)
{
    const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 12);

    for (int32 TimeStep : Ctx.TimeStepOrder)
    {
        const float X = TimeStepToScreen(Ctx, TimeStep);
        if (X < 0 || X > Ctx.AllottedGeometry.GetLocalSize().X)
        {
            continue;
        }

        const FString Label = FString::Printf(TEXT("T%d"), TimeStep);
        const float EstimatedTextWidth = Label.Len() * 8.0f;
        const float CenteredX = X + (Ctx.TimeStepWidth - EstimatedTextWidth) * 0.5f;
        const float CenteredY = (Ctx.HeaderHeight - 14.0f) * 0.5f;

        const FVector2D TextPosition(CenteredX, CenteredY);
        const FVector2D TextSize(Ctx.TimeStepWidth, Ctx.HeaderHeight);

        FSlateDrawElement::MakeText(
            Ctx.OutDrawElements,
            LayerId,
            Ctx.AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TextPosition))),
            Label,
            FontInfo,
            ESlateDrawEffect::None,
            Ctx.TextColor
        );
    }
}

void FMAGanttPainter::DrawRobotLabels(const FMAGanttPainterContext& Ctx, int32 LayerId)
{
    const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 12);

    for (int32 RobotIndex = 0; RobotIndex < Ctx.RobotIdOrder.Num(); ++RobotIndex)
    {
        const float Y = RobotIndexToScreen(Ctx, RobotIndex);
        if (Y < 0 || Y > Ctx.AllottedGeometry.GetLocalSize().Y)
        {
            continue;
        }

        const FString& RobotId = Ctx.RobotIdOrder[RobotIndex];
        const FVector2D TextPosition(5.0f, Y + Ctx.RobotRowHeight * 0.5f - 8.0f);
        const FVector2D TextSize(Ctx.LabelWidth - 10.0f, Ctx.RobotRowHeight - 4.0f);

        FSlateDrawElement::MakeText(
            Ctx.OutDrawElements,
            LayerId,
            Ctx.AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TextPosition))),
            RobotId,
            FontInfo,
            ESlateDrawEffect::None,
            Ctx.TextColor
        );
    }
}

void FMAGanttPainter::DrawTimeStepGridLines(const FMAGanttPainterContext& Ctx, int32 LayerId)
{
    FLinearColor GridColor = Ctx.GridLineColor;
    GridColor.A = 0.5f;

    const FVector2D WidgetSize = Ctx.AllottedGeometry.GetLocalSize();
    const float DataStartY = Ctx.HeaderHeight;
    const float DataEndY = WidgetSize.Y;

    for (int32 TimeStep : Ctx.TimeStepOrder)
    {
        const float X = TimeStepToScreen(Ctx, TimeStep);
        if (X < Ctx.LabelWidth || X > WidgetSize.X)
        {
            continue;
        }

        TArray<FVector2D> LinePoints;
        LinePoints.Add(FVector2D(X, DataStartY));
        LinePoints.Add(FVector2D(X, DataEndY));

        FSlateDrawElement::MakeLines(
            Ctx.OutDrawElements,
            LayerId,
            Ctx.AllottedGeometry.ToPaintGeometry(),
            LinePoints,
            ESlateDrawEffect::None,
            GridColor,
            true,
            1.0f
        );
    }

    if (Ctx.TimeStepOrder.Num() > 0)
    {
        const int32 LastTimeStep = Ctx.TimeStepOrder.Last();
        const float LastX = TimeStepToScreen(Ctx, LastTimeStep) + Ctx.TimeStepWidth;
        if (LastX >= Ctx.LabelWidth && LastX <= WidgetSize.X)
        {
            TArray<FVector2D> LinePoints;
            LinePoints.Add(FVector2D(LastX, DataStartY));
            LinePoints.Add(FVector2D(LastX, DataEndY));

            FSlateDrawElement::MakeLines(
                Ctx.OutDrawElements,
                LayerId,
                Ctx.AllottedGeometry.ToPaintGeometry(),
                LinePoints,
                ESlateDrawEffect::None,
                GridColor,
                true,
                1.0f
            );
        }
    }
}

void FMAGanttPainter::DrawSkillBars(const FMAGanttPainterContext& Ctx, int32 LayerId)
{
    const float CornerRadius = 14.0f;

    for (const FMASkillBarRenderData& RenderData : Ctx.SkillBarRenderData)
    {
        if (RenderData.Position.X + RenderData.Size.X < 0 || RenderData.Position.X > Ctx.AllottedGeometry.GetLocalSize().X)
        {
            continue;
        }
        if (RenderData.Position.Y + RenderData.Size.Y < 0 || RenderData.Position.Y > Ctx.AllottedGeometry.GetLocalSize().Y)
        {
            continue;
        }

        FSlateRoundedBoxBrush RoundedBrush(RenderData.Color, CornerRadius);
        FSlateDrawElement::MakeBox(
            Ctx.OutDrawElements,
            LayerId,
            Ctx.AllottedGeometry.ToPaintGeometry(FVector2f(RenderData.Size), FSlateLayoutTransform(FVector2f(RenderData.Position))),
            &RoundedBrush,
            ESlateDrawEffect::None,
            RenderData.Color
        );

        const float BarWidth = RenderData.Size.X;
        const float BarHeight = RenderData.Size.Y;
        const int32 TextLength = RenderData.SkillName.Len();

        const int32 MaxFontSizeByHeight = FMath::Clamp(static_cast<int32>(BarHeight * 0.4f), 8, 16);
        const float AvailableWidth = BarWidth - 8.0f;
        int32 MaxFontSizeByWidth = TextLength > 0 ? static_cast<int32>(AvailableWidth / (TextLength * 0.6f)) : MaxFontSizeByHeight;
        MaxFontSizeByWidth = FMath::Clamp(MaxFontSizeByWidth, 6, 16);

        int32 FontSize = FMath::Min(MaxFontSizeByHeight, MaxFontSizeByWidth);
        FontSize = FMath::Max(FontSize, 6);
        const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", FontSize);

        const float EstimatedTextWidth = TextLength * (FontSize * 0.6f);
        const float EstimatedTextHeight = FontSize * 1.2f;
        float CenteredX = RenderData.Position.X + (BarWidth - EstimatedTextWidth) * 0.5f;
        const float CenteredY = RenderData.Position.Y + (BarHeight - EstimatedTextHeight) * 0.5f;
        CenteredX = FMath::Max(CenteredX, RenderData.Position.X + 4.0f);

        const FVector2D TextPosition(CenteredX, CenteredY);
        const FVector2D TextSize(BarWidth - 8.0f, BarHeight - 4.0f);

        FSlateDrawElement::MakeText(
            Ctx.OutDrawElements,
            LayerId + 1,
            Ctx.AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TextPosition))),
            RenderData.SkillName,
            FontInfo,
            ESlateDrawEffect::None,
            Ctx.TextOnBarColor
        );

        if (RenderData.bIsSelected)
        {
            TArray<FVector2D> BorderPoints;
            BorderPoints.Add(RenderData.Position);
            BorderPoints.Add(RenderData.Position + FVector2D(RenderData.Size.X, 0));
            BorderPoints.Add(RenderData.Position + RenderData.Size);
            BorderPoints.Add(RenderData.Position + FVector2D(0, RenderData.Size.Y));
            BorderPoints.Add(RenderData.Position);

            FSlateDrawElement::MakeLines(
                Ctx.OutDrawElements,
                LayerId + 2,
                Ctx.AllottedGeometry.ToPaintGeometry(),
                BorderPoints,
                ESlateDrawEffect::None,
                Ctx.SelectionColor,
                true,
                2.0f
            );
        }
    }
}

void FMAGanttPainter::DrawDragPreview(const FMAGanttPainterContext& Ctx, const FMAGanttPainterDragVisual& DragVisual, int32 LayerId)
{
    if (!DragVisual.bIsDragging || DragVisual.DragPreview == nullptr)
    {
        return;
    }

    const FGanttDragPreview& DragPreview = *DragVisual.DragPreview;
    if (DragPreview.Size.X <= 0 || DragPreview.Size.Y <= 0)
    {
        return;
    }

    const FVector2D WidgetSize = Ctx.AllottedGeometry.GetLocalSize();
    if (DragPreview.Position.X + DragPreview.Size.X < 0 || DragPreview.Position.X > WidgetSize.X ||
        DragPreview.Position.Y + DragPreview.Size.Y < 0 || DragPreview.Position.Y > WidgetSize.Y)
    {
        return;
    }

    const float CornerRadius = 14.0f;
    FSlateRoundedBoxBrush RoundedBrush(DragPreview.Color, CornerRadius);
    FSlateDrawElement::MakeBox(
        Ctx.OutDrawElements,
        LayerId,
        Ctx.AllottedGeometry.ToPaintGeometry(FVector2f(DragPreview.Size), FSlateLayoutTransform(FVector2f(DragPreview.Position))),
        &RoundedBrush,
        ESlateDrawEffect::None,
        DragPreview.Color
    );

    FLinearColor BorderColor = DragPreview.Color;
    BorderColor.A = FMath::Min(1.0f, DragPreview.Color.A + 0.3f);

    TArray<FVector2D> BorderPoints;
    BorderPoints.Add(DragPreview.Position);
    BorderPoints.Add(DragPreview.Position + FVector2D(DragPreview.Size.X, 0));
    BorderPoints.Add(DragPreview.Position + DragPreview.Size);
    BorderPoints.Add(DragPreview.Position + FVector2D(0, DragPreview.Size.Y));
    BorderPoints.Add(DragPreview.Position);

    FSlateDrawElement::MakeLines(
        Ctx.OutDrawElements,
        LayerId + 1,
        Ctx.AllottedGeometry.ToPaintGeometry(),
        BorderPoints,
        ESlateDrawEffect::None,
        BorderColor,
        true,
        2.0f
    );

    if (!DragPreview.SkillName.IsEmpty())
    {
        const float BarWidth = DragPreview.Size.X;
        const float BarHeight = DragPreview.Size.Y;
        const int32 TextLength = DragPreview.SkillName.Len();

        const int32 MaxFontSizeByHeight = FMath::Clamp(static_cast<int32>(BarHeight * 0.35f), 6, 12);
        const float AvailableWidth = BarWidth - 8.0f;
        int32 MaxFontSizeByWidth = TextLength > 0 ? static_cast<int32>(AvailableWidth / (TextLength * 0.6f)) : MaxFontSizeByHeight;
        MaxFontSizeByWidth = FMath::Clamp(MaxFontSizeByWidth, 6, 12);

        const int32 FontSize = FMath::Max(FMath::Min(MaxFontSizeByHeight, MaxFontSizeByWidth), 6);
        const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", FontSize);

        const float EstimatedTextWidth = TextLength * (FontSize * 0.6f);
        float CenteredX = DragPreview.Position.X + (BarWidth - EstimatedTextWidth) * 0.5f;
        CenteredX = FMath::Max(CenteredX, DragPreview.Position.X + 4.0f);

        const FVector2D TextPosition(CenteredX, DragPreview.Position.Y + DragPreview.Size.Y * 0.5f - FontSize * 0.6f);
        const FVector2D TextSize(BarWidth - 8.0f, DragPreview.Size.Y - 4.0f);

        FSlateDrawElement::MakeText(
            Ctx.OutDrawElements,
            LayerId + 2,
            Ctx.AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TextPosition))),
            DragPreview.SkillName,
            FontInfo,
            ESlateDrawEffect::None,
            Ctx.TextOnBarColor
        );
    }
}

void FMAGanttPainter::DrawDragSourcePlaceholder(const FMAGanttPainterContext& Ctx, const FMAGanttPainterDragVisual& DragVisual, int32 LayerId)
{
    if (!DragVisual.bIsDragging || DragVisual.DragSource == nullptr)
    {
        return;
    }

    const FGanttDragSource& DragSource = *DragVisual.DragSource;
    if (!DragSource.IsValid())
    {
        return;
    }

    const float SkillBarPadding = 10.0f;
    const FVector2D PlaceholderPosition = DragSource.OriginalPosition;
    const FVector2D PlaceholderSize(Ctx.TimeStepWidth - SkillBarPadding, Ctx.RobotRowHeight - SkillBarPadding);

    const FVector2D WidgetSize = Ctx.AllottedGeometry.GetLocalSize();
    if (PlaceholderPosition.X + PlaceholderSize.X < 0 || PlaceholderPosition.X > WidgetSize.X ||
        PlaceholderPosition.Y + PlaceholderSize.Y < 0 || PlaceholderPosition.Y > WidgetSize.Y)
    {
        return;
    }

    const float DashLength = 8.0f;
    const float GapLength = 4.0f;
    const float TotalLength = DashLength + GapLength;

    float CurrentX = PlaceholderPosition.X;
    const float EndX = PlaceholderPosition.X + PlaceholderSize.X;
    while (CurrentX < EndX)
    {
        const float SegmentEnd = FMath::Min(CurrentX + DashLength, EndX);
        TArray<FVector2D> DashPoints;
        DashPoints.Add(FVector2D(CurrentX, PlaceholderPosition.Y));
        DashPoints.Add(FVector2D(SegmentEnd, PlaceholderPosition.Y));

        FSlateDrawElement::MakeLines(
            Ctx.OutDrawElements,
            LayerId,
            Ctx.AllottedGeometry.ToPaintGeometry(),
            DashPoints,
            ESlateDrawEffect::None,
            Ctx.PlaceholderColor,
            true,
            2.0f
        );
        CurrentX += TotalLength;
    }

    CurrentX = PlaceholderPosition.X;
    const float BottomY = PlaceholderPosition.Y + PlaceholderSize.Y;
    while (CurrentX < EndX)
    {
        const float SegmentEnd = FMath::Min(CurrentX + DashLength, EndX);
        TArray<FVector2D> DashPoints;
        DashPoints.Add(FVector2D(CurrentX, BottomY));
        DashPoints.Add(FVector2D(SegmentEnd, BottomY));

        FSlateDrawElement::MakeLines(
            Ctx.OutDrawElements,
            LayerId,
            Ctx.AllottedGeometry.ToPaintGeometry(),
            DashPoints,
            ESlateDrawEffect::None,
            Ctx.PlaceholderColor,
            true,
            2.0f
        );
        CurrentX += TotalLength;
    }

    float CurrentY = PlaceholderPosition.Y;
    const float EndY = PlaceholderPosition.Y + PlaceholderSize.Y;
    while (CurrentY < EndY)
    {
        const float SegmentEnd = FMath::Min(CurrentY + DashLength, EndY);
        TArray<FVector2D> DashPoints;
        DashPoints.Add(FVector2D(PlaceholderPosition.X, CurrentY));
        DashPoints.Add(FVector2D(PlaceholderPosition.X, SegmentEnd));

        FSlateDrawElement::MakeLines(
            Ctx.OutDrawElements,
            LayerId,
            Ctx.AllottedGeometry.ToPaintGeometry(),
            DashPoints,
            ESlateDrawEffect::None,
            Ctx.PlaceholderColor,
            true,
            2.0f
        );
        CurrentY += TotalLength;
    }

    CurrentY = PlaceholderPosition.Y;
    const float RightX = PlaceholderPosition.X + PlaceholderSize.X;
    while (CurrentY < EndY)
    {
        const float SegmentEnd = FMath::Min(CurrentY + DashLength, EndY);
        TArray<FVector2D> DashPoints;
        DashPoints.Add(FVector2D(RightX, CurrentY));
        DashPoints.Add(FVector2D(RightX, SegmentEnd));

        FSlateDrawElement::MakeLines(
            Ctx.OutDrawElements,
            LayerId,
            Ctx.AllottedGeometry.ToPaintGeometry(),
            DashPoints,
            ESlateDrawEffect::None,
            Ctx.PlaceholderColor,
            true,
            2.0f
        );
        CurrentY += TotalLength;
    }
}

void FMAGanttPainter::DrawDropIndicator(const FMAGanttPainterContext& Ctx, const FMAGanttPainterDragVisual& DragVisual, int32 LayerId)
{
    if (!DragVisual.bIsDragging || DragVisual.DropTarget == nullptr)
    {
        return;
    }

    const FGanttDropTarget& CurrentDropTarget = *DragVisual.DropTarget;
    if (!CurrentDropTarget.HasTarget())
    {
        return;
    }

    const int32 RobotIndex = Ctx.RobotIdOrder.IndexOfByKey(CurrentDropTarget.RobotId);
    if (RobotIndex == INDEX_NONE)
    {
        return;
    }

    const float SkillBarPadding = 10.0f;
    const FVector2D IndicatorPosition(
        TimeStepToScreen(Ctx, CurrentDropTarget.TimeStep) + SkillBarPadding * 0.5f,
        RobotIndexToScreen(Ctx, RobotIndex) + SkillBarPadding * 0.5f
    );
    const FVector2D IndicatorSize(Ctx.TimeStepWidth - SkillBarPadding, Ctx.RobotRowHeight - SkillBarPadding);

    const FVector2D WidgetSize = Ctx.AllottedGeometry.GetLocalSize();
    if (IndicatorPosition.X + IndicatorSize.X < 0 || IndicatorPosition.X > WidgetSize.X ||
        IndicatorPosition.Y + IndicatorSize.Y < 0 || IndicatorPosition.Y > WidgetSize.Y)
    {
        return;
    }

    const FLinearColor IndicatorColor = CurrentDropTarget.bIsValid ? Ctx.ValidDropColor : Ctx.InvalidDropColor;
    FLinearColor FillColor = IndicatorColor;
    FillColor.A = 0.2f;

    FSlateDrawElement::MakeBox(
        Ctx.OutDrawElements,
        LayerId,
        Ctx.AllottedGeometry.ToPaintGeometry(FVector2f(IndicatorSize), FSlateLayoutTransform(FVector2f(IndicatorPosition))),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        FillColor
    );

    TArray<FVector2D> BorderPoints;
    BorderPoints.Add(IndicatorPosition);
    BorderPoints.Add(IndicatorPosition + FVector2D(IndicatorSize.X, 0));
    BorderPoints.Add(IndicatorPosition + IndicatorSize);
    BorderPoints.Add(IndicatorPosition + FVector2D(0, IndicatorSize.Y));
    BorderPoints.Add(IndicatorPosition);

    FSlateDrawElement::MakeLines(
        Ctx.OutDrawElements,
        LayerId + 1,
        Ctx.AllottedGeometry.ToPaintGeometry(),
        BorderPoints,
        ESlateDrawEffect::None,
        IndicatorColor,
        true,
        3.0f
    );

    if (CurrentDropTarget.bIsValid && CurrentDropTarget.bIsSnapped)
    {
        const FVector2D InnerOffset(4.0f, 4.0f);
        const FVector2D InnerPosition = IndicatorPosition + InnerOffset;
        const FVector2D InnerSize = IndicatorSize - InnerOffset * 2.0f;

        TArray<FVector2D> InnerBorderPoints;
        InnerBorderPoints.Add(InnerPosition);
        InnerBorderPoints.Add(InnerPosition + FVector2D(InnerSize.X, 0));
        InnerBorderPoints.Add(InnerPosition + InnerSize);
        InnerBorderPoints.Add(InnerPosition + FVector2D(0, InnerSize.Y));
        InnerBorderPoints.Add(InnerPosition);

        FLinearColor SnapHighlightColor = Ctx.ValidDropColor;
        SnapHighlightColor.A = 0.8f;

        FSlateDrawElement::MakeLines(
            Ctx.OutDrawElements,
            LayerId + 2,
            Ctx.AllottedGeometry.ToPaintGeometry(),
            InnerBorderPoints,
            ESlateDrawEffect::None,
            SnapHighlightColor,
            true,
            1.5f
        );
    }
}
