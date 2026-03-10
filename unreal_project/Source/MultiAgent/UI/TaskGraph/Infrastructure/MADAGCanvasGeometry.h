#pragma once

#include "CoreMinimal.h"
#include "UI/TaskGraph/Domain/MADAGCanvasState.h"

class UMATaskNodeWidget;

class FMADAGCanvasGeometry
{
public:
    static FVector2D CanvasToScreen(const FVector2D& CanvasPos, float ZoomLevel, const FVector2D& ViewOffset);
    static FVector2D ScreenToCanvas(const FVector2D& ScreenPos, float ZoomLevel, const FVector2D& ViewOffset);

    static void UpdateEdgePositions(
        TArray<FMAEdgeRenderData>& EdgeRenderData,
        const TMap<FString, UMATaskNodeWidget*>& NodeWidgets,
        float ZoomLevel,
        const FVector2D& ViewOffset);

    static bool IsPointOnEdge(const FVector2D& Point, const FMAEdgeRenderData& Edge, float Tolerance = 5.0f);
    static FMAEdgeRenderData* FindEdgeAtPoint(TArray<FMAEdgeRenderData>& EdgeRenderData, const FVector2D& Point, float Tolerance = 5.0f);
    static FString FindInputPortAtPoint(
        const TMap<FString, UMATaskNodeWidget*>& NodeWidgets,
        const FVector2D& ScreenPoint,
        float ZoomLevel,
        const FVector2D& ViewOffset,
        float PortHitRadius = 20.0f);
};
