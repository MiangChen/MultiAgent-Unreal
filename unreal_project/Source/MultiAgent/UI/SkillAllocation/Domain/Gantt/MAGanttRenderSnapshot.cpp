// MAGanttRenderSnapshot.cpp

#include "MAGanttRenderSnapshot.h"

FString FMAGanttRenderSnapshot::MakeSlotKey(int32 TimeStep, const FString& RobotId)
{
    return FString::Printf(TEXT("%d|%s"), TimeStep, *RobotId);
}

void FMAGanttRenderSnapshot::Reset()
{
    Bars.Empty();
    RobotOrder.Empty();
    TimeStepOrder.Empty();
    RobotIdToIndexMap.Empty();
    TimeStepToIndexMap.Empty();
    SlotIndexMap.Empty();
}

void FMAGanttRenderSnapshot::RebuildSlotIndex()
{
    SlotIndexMap.Empty(Bars.Num());

    for (int32 Index = 0; Index < Bars.Num(); ++Index)
    {
        const FMASkillBarRenderData& Bar = Bars[Index];
        SlotIndexMap.Add(MakeSlotKey(Bar.TimeStep, Bar.RobotId), Index);
    }
}

void FMAGanttRenderSnapshot::RebuildOrderIndices()
{
    RobotIdToIndexMap.Empty(RobotOrder.Num());
    for (int32 RobotIndex = 0; RobotIndex < RobotOrder.Num(); ++RobotIndex)
    {
        RobotIdToIndexMap.Add(RobotOrder[RobotIndex], RobotIndex);
    }

    TimeStepToIndexMap.Empty(TimeStepOrder.Num());
    for (int32 TimeStepIndex = 0; TimeStepIndex < TimeStepOrder.Num(); ++TimeStepIndex)
    {
        TimeStepToIndexMap.Add(TimeStepOrder[TimeStepIndex], TimeStepIndex);
    }
}

int32 FMAGanttRenderSnapshot::FindRobotIndex(const FString& RobotId) const
{
    if (RobotId.IsEmpty())
    {
        return INDEX_NONE;
    }

    if (const int32* RobotIndex = RobotIdToIndexMap.Find(RobotId))
    {
        return *RobotIndex;
    }
    return INDEX_NONE;
}

int32 FMAGanttRenderSnapshot::FindTimeStepIndex(int32 TimeStep) const
{
    if (const int32* TimeStepIndex = TimeStepToIndexMap.Find(TimeStep))
    {
        return *TimeStepIndex;
    }
    return INDEX_NONE;
}

bool FMAGanttRenderSnapshot::HasTimeStep(int32 TimeStep) const
{
    return TimeStepToIndexMap.Contains(TimeStep);
}

bool FMAGanttRenderSnapshot::IsValidRobotIndex(int32 RobotIndex) const
{
    return RobotOrder.IsValidIndex(RobotIndex);
}

const FString* FMAGanttRenderSnapshot::GetRobotIdByIndex(int32 RobotIndex) const
{
    return RobotOrder.IsValidIndex(RobotIndex) ? &RobotOrder[RobotIndex] : nullptr;
}

bool FMAGanttRenderSnapshot::HasSkillAtSlot(int32 TimeStep, const FString& RobotId) const
{
    if (TimeStep < 0 || RobotId.IsEmpty())
    {
        return false;
    }

    return SlotIndexMap.Contains(MakeSlotKey(TimeStep, RobotId));
}

int32 FMAGanttRenderSnapshot::FindBarIndexAtSlot(int32 TimeStep, const FString& RobotId) const
{
    if (TimeStep < 0 || RobotId.IsEmpty())
    {
        return INDEX_NONE;
    }

    if (const int32* Index = SlotIndexMap.Find(MakeSlotKey(TimeStep, RobotId)))
    {
        return *Index;
    }
    return INDEX_NONE;
}

const FMASkillBarRenderData* FMAGanttRenderSnapshot::FindBarAtSlot(int32 TimeStep, const FString& RobotId) const
{
    const int32 Index = FindBarIndexAtSlot(TimeStep, RobotId);
    return Bars.IsValidIndex(Index) ? &Bars[Index] : nullptr;
}

FMASkillBarRenderData* FMAGanttRenderSnapshot::FindMutableBarAtSlot(int32 TimeStep, const FString& RobotId)
{
    const int32 Index = FindBarIndexAtSlot(TimeStep, RobotId);
    return Bars.IsValidIndex(Index) ? &Bars[Index] : nullptr;
}
