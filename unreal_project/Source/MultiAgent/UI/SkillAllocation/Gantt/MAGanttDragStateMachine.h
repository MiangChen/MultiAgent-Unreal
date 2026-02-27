// MAGanttDragStateMachine.h
// 甘特图拖拽状态机 - 管理拖拽状态与预览数据

#pragma once

#include "CoreMinimal.h"
#include "../../../Core/Types/MATaskGraphTypes.h"

struct FMAGanttRenderSnapshot;
struct FMAGanttGridLayout;
struct FMAGanttPainterDragVisual;

/** 拖拽输入上下文 */
struct MULTIAGENT_API FMAGanttDragStateContext
{
    const FMAGanttRenderSnapshot* RenderSnapshot = nullptr;
    const FMAGanttGridLayout* GridLayout = nullptr;

    float LabelWidth = 0.0f;
    float HeaderHeight = 0.0f;
    float TimeStepWidth = 0.0f;
    float RobotRowHeight = 0.0f;
    float SnapRange = 0.0f;

    FLinearColor PendingColor = FLinearColor::White;
    FLinearColor InProgressColor = FLinearColor::White;
    FLinearColor CompletedColor = FLinearColor::White;
    FLinearColor FailedColor = FLinearColor::White;

    FVector2D ScrollOffset = FVector2D::ZeroVector;
};

/** 鼠标按下处理结果 */
struct MULTIAGENT_API FMAGanttDragMouseDownResult
{
    bool bHandled = false;
    bool bCaptureMouse = false;
    bool bDragBlocked = false;
    bool bClearSelection = false;
    bool bSelectSkill = false;
    bool bPotentialDragStarted = false;

    int32 SelectedTimeStep = -1;
    FString SelectedRobotId;
    FGanttDragSource DragSource;
};

/** 鼠标移动处理结果 */
struct MULTIAGENT_API FMAGanttDragMouseMoveResult
{
    bool bHandled = false;
    bool bDragStarted = false;
    bool bDragUpdated = false;
    bool bHasHoverUpdate = false;
    bool bHoverSkill = false;

    int32 HoveredTimeStep = -1;
    FString HoveredRobotId;
    FGanttDragSource DragSource;
    FGanttDropTarget DropTarget;
    FGanttDragPreview DragPreview;
};

/** 鼠标抬起处理结果 */
struct MULTIAGENT_API FMAGanttDragMouseUpResult
{
    bool bHandled = false;
    bool bReleaseMouse = false;
    bool bSelectSkill = false;
    bool bHasDropAttempt = false;

    int32 SelectedTimeStep = -1;
    FString SelectedRobotId;
    FGanttDragSource DragSource;
    FGanttDropTarget DropTarget;
};

/**
 * 甘特图拖拽状态机
 *
 * 负责：
 * - Potential/Dragging/Idle 状态流转
 * - 拖拽源、放置目标与预览数据维护
 * - 鼠标输入到拖拽状态的转换
 */
struct MULTIAGENT_API FMAGanttDragStateMachine
{
public:
    bool IsDragging() const { return DragState == EGanttDragState::Dragging; }
    bool IsDragActive() const { return DragState != EGanttDragState::Idle; }
    bool IsDragEnabled() const { return bDragEnabled; }

    EGanttDragState GetState() const { return DragState; }
    const FGanttDragSource& GetDragSource() const { return DragSource; }
    const FGanttDropTarget& GetCurrentDropTarget() const { return CurrentDropTarget; }
    const FGanttDragPreview& GetDragPreview() const { return DragPreview; }

    void SetDragEnabled(bool bEnabled) { bDragEnabled = bEnabled; }
    void SetDragThreshold(float InThreshold) { DragThreshold = FMath::Max(0.0f, InThreshold); }
    void SetDragPreviewAlpha(float InAlpha) { DragPreviewAlpha = FMath::Clamp(InAlpha, 0.0f, 1.0f); }

    FMAGanttDragMouseDownResult HandleMouseButtonDown(const FVector2D& LocalPosition,
                                                      const FMAGanttDragStateContext& Context);
    FMAGanttDragMouseMoveResult HandleMouseMove(const FVector2D& LocalPosition,
                                                const FMAGanttDragStateContext& Context);
    FMAGanttDragMouseUpResult HandleMouseButtonUp(const FVector2D& LocalPosition,
                                                  const FMAGanttDragStateContext& Context);

    bool CancelDrag();
    FMAGanttPainterDragVisual GetDragVisualState() const;

    bool TryBeginPotentialDrag(const FMASkillBarRenderData& RenderData, const FVector2D& MouseDownLocalPosition);
    bool TryPromoteToDragging(const FVector2D& CurrentLocalPosition, const FVector2D& PreviewSize, const FLinearColor& PreviewBaseColor);
    bool UpdateDragging(const FVector2D& CurrentLocalPosition, const FGanttDropTarget& DropTarget);

    void Reset();

private:
    bool IsContextUsable(const FMAGanttDragStateContext& Context) const;
    bool GetSlotAtPosition(const FVector2D& Position, const FMAGanttDragStateContext& Context,
                           int32& OutTimeStep, FString& OutRobotId) const;
    bool IsSlotEmpty(int32 TimeStep, const FString& RobotId, const FMAGanttDragStateContext& Context) const;
    bool IsValidDropTarget(int32 TargetTimeStep, const FString& TargetRobotId,
                           const FMAGanttDragStateContext& Context) const;
    FVector2D GetSlotCenterPosition(int32 TimeStep, int32 RobotIndex,
                                    const FMAGanttDragStateContext& Context) const;
    FGanttDropTarget CalculateSnapTarget(const FVector2D& Position,
                                         const FMAGanttDragStateContext& Context) const;

private:
    /** 当前拖拽状态 */
    EGanttDragState DragState = EGanttDragState::Idle;

    /** 拖拽源信息 */
    FGanttDragSource DragSource;

    /** 当前放置目标 */
    FGanttDropTarget CurrentDropTarget;

    /** 拖拽预览信息 */
    FGanttDragPreview DragPreview;

    /** 鼠标按下位置（用于判断是否超过拖拽阈值） */
    FVector2D MouseDownPosition = FVector2D::ZeroVector;

    /** 拖拽是否启用 */
    bool bDragEnabled = true;

    /** 拖拽启动阈值（像素） */
    float DragThreshold = 5.0f;

    /** 拖拽预览透明度 */
    float DragPreviewAlpha = 0.7f;
};
