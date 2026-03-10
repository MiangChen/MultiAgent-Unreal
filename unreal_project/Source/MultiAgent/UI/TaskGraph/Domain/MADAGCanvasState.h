#pragma once

#include "CoreMinimal.h"
#include "MADAGCanvasState.generated.h"

USTRUCT()
struct FMAEdgeRenderData
{
    GENERATED_BODY()

    FString FromNodeId;
    FString ToNodeId;
    FString EdgeType;
    FVector2D StartPoint;
    FVector2D EndPoint;
    bool bIsHighlighted = false;

    FMAEdgeRenderData() {}

    FMAEdgeRenderData(const FString& InFrom, const FString& InTo, const FString& InType = TEXT("sequential"))
        : FromNodeId(InFrom)
        , ToNodeId(InTo)
        , EdgeType(InType)
    {
    }
};

struct FMADAGCanvasViewportState
{
    FVector2D ViewOffset = FVector2D::ZeroVector;
    float ZoomLevel = 1.0f;
    float MinZoom = 0.25f;
    float MaxZoom = 2.0f;

    FString SelectedNodeId;
    FString SelectedEdgeFrom;
    FString SelectedEdgeTo;

    bool bIsDraggingCanvas = false;
    FVector2D CanvasDragStart = FVector2D::ZeroVector;
    FVector2D CanvasDragStartOffset = FVector2D::ZeroVector;

    bool bIsDraggingEdge = false;
    FString DragSourceNodeId;
    FVector2D DragEdgeStart = FVector2D::ZeroVector;
    FVector2D DragEdgeEnd = FVector2D::ZeroVector;
    FString HighlightedTargetNodeId;
};
