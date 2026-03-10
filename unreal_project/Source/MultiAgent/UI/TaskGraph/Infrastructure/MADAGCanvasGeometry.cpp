#include "MADAGCanvasGeometry.h"
#include "UI/TaskGraph/MATaskNodeWidget.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"

FVector2D FMADAGCanvasGeometry::CanvasToScreen(const FVector2D& CanvasPos, float ZoomLevel, const FVector2D& ViewOffset)
{
    return (CanvasPos * ZoomLevel) + ViewOffset;
}

FVector2D FMADAGCanvasGeometry::ScreenToCanvas(const FVector2D& ScreenPos, float ZoomLevel, const FVector2D& ViewOffset)
{
    return (ScreenPos - ViewOffset) / ZoomLevel;
}

void FMADAGCanvasGeometry::UpdateEdgePositions(
    TArray<FMAEdgeRenderData>& EdgeRenderData,
    const TMap<FString, UMATaskNodeWidget*>& NodeWidgets,
    float ZoomLevel,
    const FVector2D& ViewOffset)
{
    for (FMAEdgeRenderData& Edge : EdgeRenderData)
    {
        UMATaskNodeWidget* const* FromWidgetPtr = NodeWidgets.Find(Edge.FromNodeId);
        UMATaskNodeWidget* const* ToWidgetPtr = NodeWidgets.Find(Edge.ToNodeId);
        if (!FromWidgetPtr || !ToWidgetPtr || !*FromWidgetPtr || !*ToWidgetPtr)
        {
            continue;
        }

        const UMATaskNodeWidget* FromWidget = *FromWidgetPtr;
        const UMATaskNodeWidget* ToWidget = *ToWidgetPtr;
        const FMATaskNodeData FromData = FromWidget->GetNodeData();
        const FMATaskNodeData ToData = ToWidget->GetNodeData();

        const FVector2D FromNodeScreen = CanvasToScreen(FromData.CanvasPosition, ZoomLevel, ViewOffset);
        const FVector2D ToNodeScreen = CanvasToScreen(ToData.CanvasPosition, ZoomLevel, ViewOffset);

        Edge.StartPoint = FromNodeScreen + FromWidget->GetOutputPortLocalPosition();
        Edge.EndPoint = ToNodeScreen + ToWidget->GetInputPortLocalPosition();
    }
}

bool FMADAGCanvasGeometry::IsPointOnEdge(const FVector2D& Point, const FMAEdgeRenderData& Edge, float Tolerance)
{
    const FVector2D LineDir = Edge.EndPoint - Edge.StartPoint;
    const float LineLength = LineDir.Size();
    if (LineLength < 1.0f)
    {
        return false;
    }

    const FVector2D Direction = LineDir / LineLength;
    const FVector2D PointToStart = Point - Edge.StartPoint;
    const float Projection = FVector2D::DotProduct(PointToStart, Direction);
    if (Projection < 0.0f || Projection > LineLength)
    {
        return false;
    }

    const FVector2D ClosestPoint = Edge.StartPoint + Direction * Projection;
    return FVector2D::Distance(Point, ClosestPoint) <= Tolerance;
}

FMAEdgeRenderData* FMADAGCanvasGeometry::FindEdgeAtPoint(TArray<FMAEdgeRenderData>& EdgeRenderData, const FVector2D& Point, float Tolerance)
{
    for (FMAEdgeRenderData& Edge : EdgeRenderData)
    {
        if (IsPointOnEdge(Point, Edge, Tolerance))
        {
            return &Edge;
        }
    }

    return nullptr;
}

FString FMADAGCanvasGeometry::FindInputPortAtPoint(
    const TMap<FString, UMATaskNodeWidget*>& NodeWidgets,
    const FVector2D& ScreenPoint,
    float ZoomLevel,
    const FVector2D& ViewOffset,
    float PortHitRadius)
{
    for (const TPair<FString, UMATaskNodeWidget*>& Pair : NodeWidgets)
    {
        const UMATaskNodeWidget* Widget = Pair.Value;
        if (!Widget)
        {
            continue;
        }

        const FMATaskNodeData NodeData = Widget->GetNodeData();
        const FVector2D NodeScreenPos = CanvasToScreen(NodeData.CanvasPosition, ZoomLevel, ViewOffset);
        const FVector2D InputPortScreen = NodeScreenPos + Widget->GetInputPortLocalPosition();
        if (FVector2D::Distance(ScreenPoint, InputPortScreen) <= PortHitRadius)
        {
            return Pair.Key;
        }
    }

    return FString();
}
