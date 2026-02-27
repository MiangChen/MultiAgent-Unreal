// MAGanttPainter.h
// 甘特图绘制器 - 负责纯 Slate 绘制逻辑

#pragma once

#include "CoreMinimal.h"
#include "../../../Core/Types/MATaskGraphTypes.h"
#include "MAGanttGridLayout.h"

struct FGeometry;
class FSlateWindowElementList;

/** 甘特图绘制上下文（无业务逻辑） */
struct MULTIAGENT_API FMAGanttPainterContext
{
    const FGeometry& AllottedGeometry;
    FSlateWindowElementList& OutDrawElements;

    const TArray<int32>& TimeStepOrder;
    const TArray<FString>& RobotIdOrder;
    const TArray<FMASkillBarRenderData>& SkillBarRenderData;

    const FMAGanttGridLayout& GridLayout;

    float LabelWidth = 0.0f;
    float HeaderHeight = 0.0f;
    float TimeStepWidth = 0.0f;
    float RobotRowHeight = 0.0f;
    FVector2D ScrollOffset = FVector2D::ZeroVector;

    FLinearColor GridLineColor = FLinearColor::White;
    FLinearColor TextColor = FLinearColor::White;
    FLinearColor SelectionColor = FLinearColor::White;
    FLinearColor ValidDropColor = FLinearColor::White;
    FLinearColor InvalidDropColor = FLinearColor::White;
    FLinearColor PlaceholderColor = FLinearColor::White;
    FLinearColor TextOnBarColor = FLinearColor::White;
};

/** 拖拽视觉状态（只读） */
struct MULTIAGENT_API FMAGanttPainterDragVisual
{
    bool bIsDragging = false;
    const FGanttDragSource* DragSource = nullptr;
    const FGanttDropTarget* DropTarget = nullptr;
    const FGanttDragPreview* DragPreview = nullptr;
};

/** 甘特图绘制器 */
class MULTIAGENT_API FMAGanttPainter
{
public:
    static void DrawTimelineHeader(const FMAGanttPainterContext& Ctx, int32 LayerId);
    static void DrawRobotLabels(const FMAGanttPainterContext& Ctx, int32 LayerId);
    static void DrawTimeStepGridLines(const FMAGanttPainterContext& Ctx, int32 LayerId);
    static void DrawSkillBars(const FMAGanttPainterContext& Ctx, int32 LayerId);

    static void DrawDragPreview(const FMAGanttPainterContext& Ctx, const FMAGanttPainterDragVisual& DragVisual, int32 LayerId);
    static void DrawDragSourcePlaceholder(const FMAGanttPainterContext& Ctx, const FMAGanttPainterDragVisual& DragVisual, int32 LayerId);
    static void DrawDropIndicator(const FMAGanttPainterContext& Ctx, const FMAGanttPainterDragVisual& DragVisual, int32 LayerId);

private:
    static float TimeStepToScreen(const FMAGanttPainterContext& Ctx, int32 TimeStep);
    static float RobotIndexToScreen(const FMAGanttPainterContext& Ctx, int32 RobotIndex);
};
