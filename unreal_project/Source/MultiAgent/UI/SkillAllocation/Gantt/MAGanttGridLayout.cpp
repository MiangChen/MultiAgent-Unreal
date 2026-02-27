// MAGanttGridLayout.cpp

#include "MAGanttGridLayout.h"

void FMAGanttGridLayout::SetTimeStepOrder(const TArray<int32>& InTimeStepOrder)
{
    TimeStepOrder = InTimeStepOrder;
    TimeStepToColumnIndex.Empty(TimeStepOrder.Num());

    for (int32 ColumnIndex = 0; ColumnIndex < TimeStepOrder.Num(); ++ColumnIndex)
    {
        TimeStepToColumnIndex.Add(TimeStepOrder[ColumnIndex], ColumnIndex);
    }
}

void FMAGanttGridLayout::Reset()
{
    TimeStepOrder.Empty();
    TimeStepToColumnIndex.Empty();
}

float FMAGanttGridLayout::TimeStepToScreen(int32 TimeStep, float LabelWidth, float TimeStepWidth, float ScrollX) const
{
    const int32* ColumnIndex = TimeStepToColumnIndex.Find(TimeStep);
    if (!ColumnIndex)
    {
        // Unknown timestep: return an out-of-view x so caller-side clipping ignores it.
        return LabelWidth - TimeStepWidth - ScrollX;
    }

    return LabelWidth + (*ColumnIndex) * TimeStepWidth - ScrollX;
}

int32 FMAGanttGridLayout::ScreenToTimeStep(float ScreenX, float LabelWidth, float TimeStepWidth, float ScrollX) const
{
    if (TimeStepOrder.Num() == 0 || TimeStepWidth <= KINDA_SMALL_NUMBER)
    {
        return -1;
    }

    const float AdjustedX = ScreenX - LabelWidth + ScrollX;
    const int32 ColumnIndex = FMath::FloorToInt(AdjustedX / TimeStepWidth);
    if (ColumnIndex < 0 || ColumnIndex >= TimeStepOrder.Num())
    {
        return -1;
    }

    return TimeStepOrder[ColumnIndex];
}

float FMAGanttGridLayout::RobotIndexToScreen(int32 RobotIndex, float HeaderHeight, float RobotRowHeight, float ScrollY) const
{
    return HeaderHeight + RobotIndex * RobotRowHeight - ScrollY;
}

int32 FMAGanttGridLayout::ScreenToRobotIndex(float ScreenY, float HeaderHeight, float RobotRowHeight, float ScrollY) const
{
    if (RobotRowHeight <= KINDA_SMALL_NUMBER)
    {
        return -1;
    }

    const float AdjustedY = ScreenY - HeaderHeight + ScrollY;
    return FMath::FloorToInt(AdjustedY / RobotRowHeight);
}

FVector2D FMAGanttGridLayout::GetSlotCenterPosition(int32 TimeStep, int32 RobotIndex,
                                                     float LabelWidth, float HeaderHeight,
                                                     float TimeStepWidth, float RobotRowHeight,
                                                     const FVector2D& ScrollOffset) const
{
    const float SlotX = TimeStepToScreen(TimeStep, LabelWidth, TimeStepWidth, ScrollOffset.X);
    const float SlotY = RobotIndexToScreen(RobotIndex, HeaderHeight, RobotRowHeight, ScrollOffset.Y);

    return FVector2D(
        SlotX + TimeStepWidth * 0.5f,
        SlotY + RobotRowHeight * 0.5f
    );
}
