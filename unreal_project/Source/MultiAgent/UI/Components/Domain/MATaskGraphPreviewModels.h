#pragma once

#include "CoreMinimal.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"

struct FMATaskGraphPreviewNodeModel
{
    FString NodeId;
    FString Description;
    FString Location;
    EMATaskExecutionStatus Status = EMATaskExecutionStatus::Pending;
    int32 Level = 0;
};

struct FMATaskGraphPreviewEdgeModel
{
    FString FromNodeId;
    FString ToNodeId;
    FString EdgeType;
};

struct FMATaskGraphPreviewModel
{
    bool bHasData = false;
    FString StatsText = TEXT("No data");
    TArray<FMATaskGraphPreviewNodeModel> Nodes;
    TArray<FMATaskGraphPreviewEdgeModel> Edges;
};

struct FMATaskGraphPreviewNodeRenderData
{
    FString NodeId;
    FString Description;
    FString Location;
    FVector2D Position = FVector2D::ZeroVector;
    FVector2D Size = FVector2D(60.0f, 30.0f);
    EMATaskExecutionStatus Status = EMATaskExecutionStatus::Pending;
    int32 Level = 0;
};

struct FMATaskGraphPreviewEdgeRenderData
{
    FVector2D StartPoint = FVector2D::ZeroVector;
    FVector2D EndPoint = FVector2D::ZeroVector;
    FString EdgeType;
};

struct FMATaskGraphPreviewLayoutConfig
{
    float NodeWidth = 50.0f;
    float NodeHeight = 24.0f;
    float TopMargin = 25.0f;
    float LeftMargin = 10.0f;
};

struct FMATaskGraphPreviewLayoutResult
{
    TArray<FMATaskGraphPreviewNodeRenderData> Nodes;
    TArray<FMATaskGraphPreviewEdgeRenderData> Edges;
};
