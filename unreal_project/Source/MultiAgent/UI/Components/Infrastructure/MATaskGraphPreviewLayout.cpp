#include "MATaskGraphPreviewLayout.h"

FMATaskGraphPreviewLayoutResult FMATaskGraphPreviewLayout::BuildLayout(
    const FMATaskGraphPreviewModel& Model,
    const FVector2D& CanvasSize,
    const FMATaskGraphPreviewLayoutConfig& Config)
{
    FMATaskGraphPreviewLayoutResult Result;
    if (!Model.bHasData || Model.Nodes.Num() == 0)
    {
        return Result;
    }

    Result.Nodes.Reserve(Model.Nodes.Num());
    for (const FMATaskGraphPreviewNodeModel& Node : Model.Nodes)
    {
        FMATaskGraphPreviewNodeRenderData RenderData;
        RenderData.NodeId = Node.NodeId;
        RenderData.Description = Node.Description;
        RenderData.Location = Node.Location;
        RenderData.Status = Node.Status;
        RenderData.Level = Node.Level;
        RenderData.Size = FVector2D(Config.NodeWidth, Config.NodeHeight);
        Result.Nodes.Add(MoveTemp(RenderData));
    }

    float AvailableWidth = CanvasSize.X - Config.LeftMargin * 2.0f;
    float AvailableHeight = CanvasSize.Y - Config.TopMargin - 10.0f;

    TMap<int32, TArray<int32>> LevelNodes;
    int32 MaxLevel = 0;
    for (int32 Index = 0; Index < Result.Nodes.Num(); ++Index)
    {
        const int32 Level = Result.Nodes[Index].Level;
        LevelNodes.FindOrAdd(Level).Add(Index);
        MaxLevel = FMath::Max(MaxLevel, Level);
    }

    const float LevelWidth = (MaxLevel > 0) ? AvailableWidth / static_cast<float>(MaxLevel + 1) : AvailableWidth;
    for (TPair<int32, TArray<int32>>& Pair : LevelNodes)
    {
        const int32 NodesInLevel = Pair.Value.Num();
        const float LevelHeight = (NodesInLevel > 1) ? AvailableHeight / static_cast<float>(NodesInLevel) : AvailableHeight;

        for (int32 LocalIndex = 0; LocalIndex < Pair.Value.Num(); ++LocalIndex)
        {
            const int32 NodeIndex = Pair.Value[LocalIndex];
            const float X = Config.LeftMargin + Pair.Key * LevelWidth + LevelWidth / 2.0f - Config.NodeWidth / 2.0f;
            const float Y = Config.TopMargin + LocalIndex * LevelHeight + LevelHeight / 2.0f - Config.NodeHeight / 2.0f;
            Result.Nodes[NodeIndex].Position = FVector2D(X, Y);
        }
    }

    Result.Edges.Reserve(Model.Edges.Num());
    for (const FMATaskGraphPreviewEdgeModel& Edge : Model.Edges)
    {
        const FMATaskGraphPreviewNodeRenderData* FromNode = nullptr;
        const FMATaskGraphPreviewNodeRenderData* ToNode = nullptr;
        for (const FMATaskGraphPreviewNodeRenderData& Node : Result.Nodes)
        {
            if (!FromNode && Node.NodeId == Edge.FromNodeId)
            {
                FromNode = &Node;
            }
            if (!ToNode && Node.NodeId == Edge.ToNodeId)
            {
                ToNode = &Node;
            }
        }

        if (!FromNode || !ToNode)
        {
            continue;
        }

        FMATaskGraphPreviewEdgeRenderData EdgeRender;
        EdgeRender.EdgeType = Edge.EdgeType;
        EdgeRender.StartPoint = FromNode->Position + FVector2D(Config.NodeWidth, Config.NodeHeight / 2.0f);
        EdgeRender.EndPoint = ToNode->Position + FVector2D(0.0f, Config.NodeHeight / 2.0f);
        Result.Edges.Add(MoveTemp(EdgeRender));
    }

    return Result;
}
