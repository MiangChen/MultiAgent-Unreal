#include "MASkillListPreviewLayout.h"

FMASkillListPreviewLayoutResult FMASkillListPreviewLayout::BuildLayout(
    const FMASkillListPreviewModel& Model,
    const FVector2D& CanvasSize,
    const FMASkillListPreviewLayoutConfig& Config)
{
    FMASkillListPreviewLayoutResult Result;
    if (!Model.bHasData || Model.Bars.Num() == 0)
    {
        return Result;
    }

    const float AvailableWidth = CanvasSize.X - Config.LeftMargin - Config.RightMargin;
    const float AvailableHeight = CanvasSize.Y - Config.TopMargin - Config.BottomMargin;
    const float TimeStepWidth = Model.TimeSteps.Num() > 0
        ? AvailableWidth / static_cast<float>(Model.TimeSteps.Num())
        : AvailableWidth;
    const float ActualRowHeight = Model.RobotIds.Num() > 0
        ? FMath::Min(Config.RowHeight, AvailableHeight / static_cast<float>(Model.RobotIds.Num()))
        : Config.RowHeight;

    Result.Bars.Reserve(Model.Bars.Num());
    for (const FMASkillListPreviewBarModel& Bar : Model.Bars)
    {
        const int32 TimeStepIndex = Model.TimeSteps.IndexOfByKey(Bar.TimeStep);
        if (TimeStepIndex == INDEX_NONE)
        {
            continue;
        }

        FMASkillListPreviewBarRenderData RenderBar;
        RenderBar.RobotId = Bar.RobotId;
        RenderBar.SkillName = Bar.SkillName;
        RenderBar.TimeStep = Bar.TimeStep;
        RenderBar.RobotIndex = Bar.RobotIndex;
        RenderBar.Status = Bar.Status;
        RenderBar.Position = FVector2D(
            Config.LeftMargin + TimeStepIndex * TimeStepWidth + Config.BarPadding,
            Config.TopMargin + Bar.RobotIndex * ActualRowHeight + Config.BarPadding);
        RenderBar.Size = FVector2D(
            TimeStepWidth - Config.BarPadding * 2.0f,
            ActualRowHeight - Config.BarPadding * 2.0f);
        Result.Bars.Add(MoveTemp(RenderBar));
    }

    return Result;
}
