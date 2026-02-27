// MAGanttDragStateMachine.cpp

#include "MAGanttDragStateMachine.h"

#include "MAGanttGridLayout.h"
#include "MAGanttPainter.h"
#include "MAGanttRenderSnapshot.h"

namespace
{
constexpr float GanttSkillBarPadding = 10.0f;

const FMASkillBarRenderData* FindSkillBarAtPosition(const FVector2D& Position, const FMAGanttDragStateContext& Context)
{
    if (!Context.RenderSnapshot)
    {
        return nullptr;
    }

    for (const FMASkillBarRenderData& RenderData : Context.RenderSnapshot->Bars)
    {
        const FVector2D BarMin = RenderData.Position;
        const FVector2D BarMax = RenderData.Position + RenderData.Size;

        if (Position.X >= BarMin.X && Position.X <= BarMax.X &&
            Position.Y >= BarMin.Y && Position.Y <= BarMax.Y)
        {
            return &RenderData;
        }
    }

    return nullptr;
}

FLinearColor ResolveStatusColor(ESkillExecutionStatus Status, const FMAGanttDragStateContext& Context)
{
    switch (Status)
    {
        case ESkillExecutionStatus::Pending:
            return Context.PendingColor;
        case ESkillExecutionStatus::InProgress:
            return Context.InProgressColor;
        case ESkillExecutionStatus::Completed:
            return Context.CompletedColor;
        case ESkillExecutionStatus::Failed:
            return Context.FailedColor;
        default:
            return Context.PendingColor;
    }
}
}

FMAGanttDragMouseDownResult FMAGanttDragStateMachine::HandleMouseButtonDown(const FVector2D& LocalPosition,
                                                                             const FMAGanttDragStateContext& Context)
{
    FMAGanttDragMouseDownResult Result;
    Result.bHandled = true;

    const FMASkillBarRenderData* HitSkillBar = FindSkillBarAtPosition(LocalPosition, Context);
    if (!HitSkillBar)
    {
        Result.bClearSelection = true;
        return Result;
    }

    if (IsDragEnabled() && TryBeginPotentialDrag(*HitSkillBar, LocalPosition))
    {
        Result.bCaptureMouse = true;
        Result.bPotentialDragStarted = true;
        Result.DragSource = GetDragSource();
        return Result;
    }

    if (!IsDragEnabled())
    {
        Result.bDragBlocked = true;
    }

    Result.bSelectSkill = true;
    Result.SelectedTimeStep = HitSkillBar->TimeStep;
    Result.SelectedRobotId = HitSkillBar->RobotId;
    return Result;
}

FMAGanttDragMouseMoveResult FMAGanttDragStateMachine::HandleMouseMove(const FVector2D& LocalPosition,
                                                                       const FMAGanttDragStateContext& Context)
{
    FMAGanttDragMouseMoveResult Result;

    if (GetState() == EGanttDragState::Potential)
    {
        const FVector2D PreviewSize(
            FMath::Max(1.0f, Context.TimeStepWidth - GanttSkillBarPadding),
            FMath::Max(1.0f, Context.RobotRowHeight - GanttSkillBarPadding));
        const FLinearColor PreviewBaseColor = ResolveStatusColor(GetDragSource().Status, Context);

        Result.bHandled = true;
        if (TryPromoteToDragging(LocalPosition, PreviewSize, PreviewBaseColor))
        {
            Result.bDragStarted = true;
            Result.DragSource = GetDragSource();
        }
        return Result;
    }

    if (IsDragging())
    {
        const FGanttDropTarget CalculatedDropTarget = CalculateSnapTarget(LocalPosition, Context);
        UpdateDragging(LocalPosition, CalculatedDropTarget);

        Result.bHandled = true;
        Result.bDragUpdated = true;
        Result.DropTarget = CalculatedDropTarget;
        Result.DragPreview = GetDragPreview();
        return Result;
    }

    Result.bHasHoverUpdate = true;
    if (const FMASkillBarRenderData* HoverSkillBar = FindSkillBarAtPosition(LocalPosition, Context))
    {
        Result.bHoverSkill = true;
        Result.HoveredTimeStep = HoverSkillBar->TimeStep;
        Result.HoveredRobotId = HoverSkillBar->RobotId;
    }

    return Result;
}

FMAGanttDragMouseUpResult FMAGanttDragStateMachine::HandleMouseButtonUp(const FVector2D& LocalPosition,
                                                                         const FMAGanttDragStateContext& Context)
{
    FMAGanttDragMouseUpResult Result;

    if (GetState() == EGanttDragState::Potential)
    {
        Result.bHandled = true;
        Result.bReleaseMouse = true;
        Result.bSelectSkill = true;
        Result.SelectedTimeStep = DragSource.TimeStep;
        Result.SelectedRobotId = DragSource.RobotId;
        Reset();
        return Result;
    }

    if (IsDragging())
    {
        Result.bHandled = true;
        Result.bReleaseMouse = true;
        Result.bHasDropAttempt = true;
        Result.DragSource = DragSource;
        Result.DropTarget = CalculateSnapTarget(LocalPosition, Context);
        Reset();
        return Result;
    }

    return Result;
}

bool FMAGanttDragStateMachine::CancelDrag()
{
    if (!IsDragActive())
    {
        return false;
    }

    Reset();
    return true;
}

FMAGanttPainterDragVisual FMAGanttDragStateMachine::GetDragVisualState() const
{
    FMAGanttPainterDragVisual DragVisual;
    DragVisual.bIsDragging = IsDragging();
    DragVisual.DragSource = &GetDragSource();
    DragVisual.DropTarget = &GetCurrentDropTarget();
    DragVisual.DragPreview = &GetDragPreview();
    return DragVisual;
}

bool FMAGanttDragStateMachine::TryBeginPotentialDrag(const FMASkillBarRenderData& RenderData, const FVector2D& MouseDownLocalPosition)
{
    if (!bDragEnabled)
    {
        return false;
    }

    DragState = EGanttDragState::Potential;
    MouseDownPosition = MouseDownLocalPosition;

    DragSource.TimeStep = RenderData.TimeStep;
    DragSource.RobotId = RenderData.RobotId;
    DragSource.SkillName = RenderData.SkillName;
    DragSource.ParamsJson = RenderData.ParamsJson;
    DragSource.Status = RenderData.Status;
    DragSource.OriginalPosition = RenderData.Position;

    CurrentDropTarget.Reset();
    DragPreview.Reset();
    return true;
}

bool FMAGanttDragStateMachine::TryPromoteToDragging(const FVector2D& CurrentLocalPosition, const FVector2D& PreviewSize, const FLinearColor& PreviewBaseColor)
{
    if (DragState != EGanttDragState::Potential)
    {
        return false;
    }

    const float MoveDistance = FVector2D::Distance(CurrentLocalPosition, MouseDownPosition);
    if (MoveDistance < DragThreshold)
    {
        return false;
    }

    DragState = EGanttDragState::Dragging;
    DragPreview.Position = CurrentLocalPosition;
    DragPreview.Size = PreviewSize;
    DragPreview.Color = PreviewBaseColor;
    DragPreview.Color.A = DragPreviewAlpha;
    DragPreview.SkillName = DragSource.SkillName;

    return true;
}

bool FMAGanttDragStateMachine::UpdateDragging(const FVector2D& CurrentLocalPosition, const FGanttDropTarget& DropTarget)
{
    if (DragState != EGanttDragState::Dragging)
    {
        return false;
    }

    CurrentDropTarget = DropTarget;

    const FVector2D PreviewHalfSize = DragPreview.Size * 0.5f;
    if (CurrentDropTarget.bIsSnapped)
    {
        DragPreview.Position = CurrentDropTarget.SnapPosition - PreviewHalfSize;
    }
    else
    {
        DragPreview.Position = CurrentLocalPosition - PreviewHalfSize;
    }

    return true;
}

void FMAGanttDragStateMachine::Reset()
{
    DragState = EGanttDragState::Idle;
    DragSource.Reset();
    CurrentDropTarget.Reset();
    DragPreview.Reset();
    MouseDownPosition = FVector2D::ZeroVector;
}

bool FMAGanttDragStateMachine::IsContextUsable(const FMAGanttDragStateContext& Context) const
{
    return Context.RenderSnapshot != nullptr &&
           Context.GridLayout != nullptr &&
           Context.TimeStepWidth > KINDA_SMALL_NUMBER &&
           Context.RobotRowHeight > KINDA_SMALL_NUMBER;
}

bool FMAGanttDragStateMachine::GetSlotAtPosition(const FVector2D& Position, const FMAGanttDragStateContext& Context,
                                                 int32& OutTimeStep, FString& OutRobotId) const
{
    OutTimeStep = -1;
    OutRobotId.Empty();

    if (!IsContextUsable(Context))
    {
        return false;
    }

    if (Position.X < Context.LabelWidth || Position.Y < Context.HeaderHeight)
    {
        return false;
    }

    const int32 TimeStep = Context.GridLayout->ScreenToTimeStep(
        Position.X, Context.LabelWidth, Context.TimeStepWidth, Context.ScrollOffset.X);
    const int32 RobotIndex = Context.GridLayout->ScreenToRobotIndex(
        Position.Y, Context.HeaderHeight, Context.RobotRowHeight, Context.ScrollOffset.Y);

    if (TimeStep < 0 || !Context.RenderSnapshot->HasTimeStep(TimeStep))
    {
        return false;
    }

    if (!Context.RenderSnapshot->IsValidRobotIndex(RobotIndex))
    {
        return false;
    }

    const FString* RobotId = Context.RenderSnapshot->GetRobotIdByIndex(RobotIndex);
    if (!RobotId)
    {
        return false;
    }

    OutTimeStep = TimeStep;
    OutRobotId = *RobotId;
    return true;
}

bool FMAGanttDragStateMachine::IsSlotEmpty(int32 TimeStep, const FString& RobotId, const FMAGanttDragStateContext& Context) const
{
    if (!Context.RenderSnapshot || TimeStep < 0 || RobotId.IsEmpty())
    {
        return false;
    }

    return !Context.RenderSnapshot->HasSkillAtSlot(TimeStep, RobotId);
}

bool FMAGanttDragStateMachine::IsValidDropTarget(int32 TargetTimeStep, const FString& TargetRobotId,
                                                 const FMAGanttDragStateContext& Context) const
{
    if (TargetTimeStep < 0 || TargetRobotId.IsEmpty())
    {
        return false;
    }

    if (DragSource.IsValid() && TargetTimeStep == DragSource.TimeStep && TargetRobotId == DragSource.RobotId)
    {
        return false;
    }

    return IsSlotEmpty(TargetTimeStep, TargetRobotId, Context);
}

FVector2D FMAGanttDragStateMachine::GetSlotCenterPosition(int32 TimeStep, int32 RobotIndex,
                                                          const FMAGanttDragStateContext& Context) const
{
    if (!IsContextUsable(Context))
    {
        return FVector2D::ZeroVector;
    }

    return Context.GridLayout->GetSlotCenterPosition(
        TimeStep,
        RobotIndex,
        Context.LabelWidth,
        Context.HeaderHeight,
        Context.TimeStepWidth,
        Context.RobotRowHeight,
        Context.ScrollOffset);
}

FGanttDropTarget FMAGanttDragStateMachine::CalculateSnapTarget(const FVector2D& Position,
                                                               const FMAGanttDragStateContext& Context) const
{
    FGanttDropTarget Result;
    Result.Reset();

    if (!IsContextUsable(Context) ||
        !Context.RenderSnapshot ||
        Context.RenderSnapshot->TimeStepOrder.Num() == 0 ||
        Context.RenderSnapshot->RobotOrder.Num() == 0)
    {
        return Result;
    }

    int32 CurrentTimeStep = -1;
    FString CurrentRobotId;
    if (!GetSlotAtPosition(Position, Context, CurrentTimeStep, CurrentRobotId))
    {
        return Result;
    }

    const int32 CurrentRobotIndex = Context.RenderSnapshot->FindRobotIndex(CurrentRobotId);
    if (CurrentRobotIndex == INDEX_NONE)
    {
        return Result;
    }

    float MinDistance = FLT_MAX;
    int32 BestTimeStep = -1;
    int32 BestRobotIndex = -1;
    FVector2D BestSnapPosition = FVector2D::ZeroVector;

    TArray<TPair<int32, int32>> SearchSlots;
    SearchSlots.Reserve(5);
    SearchSlots.Add(TPair<int32, int32>(CurrentTimeStep, CurrentRobotIndex));

    const int32 TimeStepIdx = Context.RenderSnapshot->FindTimeStepIndex(CurrentTimeStep);
    if (TimeStepIdx != INDEX_NONE)
    {
        if (TimeStepIdx > 0)
        {
            SearchSlots.Add(TPair<int32, int32>(Context.RenderSnapshot->TimeStepOrder[TimeStepIdx - 1], CurrentRobotIndex));
        }
        if (TimeStepIdx < Context.RenderSnapshot->TimeStepOrder.Num() - 1)
        {
            SearchSlots.Add(TPair<int32, int32>(Context.RenderSnapshot->TimeStepOrder[TimeStepIdx + 1], CurrentRobotIndex));
        }
    }

    if (CurrentRobotIndex > 0)
    {
        SearchSlots.Add(TPair<int32, int32>(CurrentTimeStep, CurrentRobotIndex - 1));
    }
    if (CurrentRobotIndex < Context.RenderSnapshot->RobotOrder.Num() - 1)
    {
        SearchSlots.Add(TPair<int32, int32>(CurrentTimeStep, CurrentRobotIndex + 1));
    }

    for (const TPair<int32, int32>& Slot : SearchSlots)
    {
        const int32 TestTimeStep = Slot.Key;
        const int32 TestRobotIndex = Slot.Value;
        const FString* TestRobotId = Context.RenderSnapshot->GetRobotIdByIndex(TestRobotIndex);
        if (!TestRobotId)
        {
            continue;
        }

        if (!IsValidDropTarget(TestTimeStep, *TestRobotId, Context))
        {
            continue;
        }

        const FVector2D SlotCenter = GetSlotCenterPosition(TestTimeStep, TestRobotIndex, Context);
        const float Distance = FVector2D::Distance(Position, SlotCenter);

        if (Distance <= Context.SnapRange && Distance < MinDistance)
        {
            MinDistance = Distance;
            BestTimeStep = TestTimeStep;
            BestRobotIndex = TestRobotIndex;
            BestSnapPosition = SlotCenter;
        }
    }

    if (BestTimeStep >= 0 && BestRobotIndex >= 0)
    {
        const FString* BestRobotId = Context.RenderSnapshot->GetRobotIdByIndex(BestRobotIndex);
        if (BestRobotId)
        {
            Result.TimeStep = BestTimeStep;
            Result.RobotId = *BestRobotId;
            Result.bIsValid = true;
            Result.bIsSnapped = true;
            Result.SnapPosition = BestSnapPosition;
            return Result;
        }
    }

    Result.TimeStep = CurrentTimeStep;
    Result.RobotId = CurrentRobotId;
    Result.bIsValid = IsValidDropTarget(CurrentTimeStep, CurrentRobotId, Context);
    Result.bIsSnapped = false;
    Result.SnapPosition = FVector2D::ZeroVector;
    return Result;
}
