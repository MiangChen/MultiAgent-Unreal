#pragma once

#include "CoreMinimal.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"

struct FMASkillListPreviewBarModel
{
    FString RobotId;
    FString SkillName;
    int32 TimeStep = 0;
    int32 RobotIndex = 0;
    ESkillExecutionStatus Status = ESkillExecutionStatus::Pending;
};

struct FMASkillListPreviewModel
{
    bool bHasData = false;
    FString StatsText = TEXT("No data");
    TArray<FString> RobotIds;
    TArray<int32> TimeSteps;
    TArray<FMASkillListPreviewBarModel> Bars;
};

struct FMASkillListPreviewBarRenderData
{
    FString RobotId;
    FString SkillName;
    int32 TimeStep = 0;
    int32 RobotIndex = 0;
    ESkillExecutionStatus Status = ESkillExecutionStatus::Pending;
    FVector2D Position = FVector2D::ZeroVector;
    FVector2D Size = FVector2D::ZeroVector;
};

struct FMASkillListPreviewLayoutConfig
{
    float TopMargin = 25.0f;
    float LeftMargin = 60.0f;
    float RightMargin = 10.0f;
    float BottomMargin = 5.0f;
    float RowHeight = 20.0f;
    float BarPadding = 4.0f;
};

struct FMASkillListPreviewLayoutResult
{
    TArray<FMASkillListPreviewBarRenderData> Bars;
};
