#include "MATaskGraphPreviewCoordinator.h"

FMATaskGraphPreviewModel FMATaskGraphPreviewCoordinator::BuildModel(const FMATaskGraphData& Data)
{
    FMATaskGraphPreviewModel Model;
    Model.bHasData = Data.Nodes.Num() > 0;
    Model.StatsText = Model.bHasData
        ? FString::Printf(TEXT("%d nodes, %d edges"), Data.Nodes.Num(), Data.Edges.Num())
        : TEXT("No data");

    TMap<FString, int32> NodeLevels;
    for (const FMATaskNodeData& Node : Data.Nodes)
    {
        NodeLevels.Add(Node.TaskId, 0);
    }

    for (int32 Iteration = 0; Iteration < Data.Nodes.Num(); ++Iteration)
    {
        for (const FMATaskEdgeData& Edge : Data.Edges)
        {
            int32* FromLevel = NodeLevels.Find(Edge.FromNodeId);
            int32* ToLevel = NodeLevels.Find(Edge.ToNodeId);
            if (FromLevel && ToLevel)
            {
                *ToLevel = FMath::Max(*ToLevel, *FromLevel + 1);
            }
        }
    }

    Model.Nodes.Reserve(Data.Nodes.Num());
    for (const FMATaskNodeData& Node : Data.Nodes)
    {
        FMATaskGraphPreviewNodeModel NodeModel;
        NodeModel.NodeId = Node.TaskId;
        NodeModel.Description = Node.Description;
        NodeModel.Location = Node.Location;
        NodeModel.Status = Node.Status;
        NodeModel.Level = NodeLevels.FindRef(Node.TaskId);
        Model.Nodes.Add(MoveTemp(NodeModel));
    }

    Model.Edges.Reserve(Data.Edges.Num());
    for (const FMATaskEdgeData& Edge : Data.Edges)
    {
        FMATaskGraphPreviewEdgeModel EdgeModel;
        EdgeModel.FromNodeId = Edge.FromNodeId;
        EdgeModel.ToNodeId = Edge.ToNodeId;
        EdgeModel.EdgeType = Edge.EdgeType;
        Model.Edges.Add(MoveTemp(EdgeModel));
    }

    return Model;
}

FMATaskGraphPreviewModel FMATaskGraphPreviewCoordinator::MakeEmptyModel()
{
    return FMATaskGraphPreviewModel();
}
