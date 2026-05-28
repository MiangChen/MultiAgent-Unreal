#include "MADAGCanvasAutoLayout.h"

TMap<FString, FVector2D> FMADAGCanvasAutoLayout::BuildTopologicalLayout(
    const FMATaskGraphData& Data,
    float StartX,
    float StartY,
    float NodeSpacingX,
    float NodeSpacingY)
{
    TMap<FString, FVector2D> Result;
    if (Data.Nodes.IsEmpty())
    {
        return Result;
    }

    TMap<FString, TArray<FString>> Adjacency;
    TMap<FString, int32> InDegree;
    TSet<FString> AllNodeIds;

    for (const FMATaskNodeData& Node : Data.Nodes)
    {
        AllNodeIds.Add(Node.TaskId);
        Adjacency.Add(Node.TaskId, TArray<FString>());
        InDegree.Add(Node.TaskId, 0);
    }

    for (const FMATaskEdgeData& Edge : Data.Edges)
    {
        if (AllNodeIds.Contains(Edge.FromNodeId) && AllNodeIds.Contains(Edge.ToNodeId))
        {
            Adjacency.FindOrAdd(Edge.FromNodeId).Add(Edge.ToNodeId);
            InDegree.FindOrAdd(Edge.ToNodeId)++;
        }
    }

    TMap<FString, int32> NodeLevel;
    TArray<FString> Queue;
    for (const TPair<FString, int32>& Pair : InDegree)
    {
        if (Pair.Value == 0)
        {
            Queue.Add(Pair.Key);
            NodeLevel.Add(Pair.Key, 0);
        }
    }

    while (!Queue.IsEmpty())
    {
        const FString CurrentNode = Queue[0];
        Queue.RemoveAt(0);
        const int32 CurrentLevel = NodeLevel.FindRef(CurrentNode);

        for (const FString& Successor : Adjacency.FindRef(CurrentNode))
        {
            NodeLevel.FindOrAdd(Successor) = FMath::Max(NodeLevel.FindRef(Successor), CurrentLevel + 1);
            int32& SuccessorInDegree = InDegree.FindOrAdd(Successor);
            SuccessorInDegree--;
            if (SuccessorInDegree == 0)
            {
                Queue.Add(Successor);
            }
        }
    }

    for (const FString& NodeId : AllNodeIds)
    {
        NodeLevel.FindOrAdd(NodeId, 0);
    }

    TMap<int32, TArray<FString>> LevelNodes;
    int32 MaxLevel = 0;
    for (const TPair<FString, int32>& Pair : NodeLevel)
    {
        LevelNodes.FindOrAdd(Pair.Value).Add(Pair.Key);
        MaxLevel = FMath::Max(MaxLevel, Pair.Value);
    }

    for (int32 Level = 0; Level <= MaxLevel; ++Level)
    {
        const TArray<FString>* NodesInLevel = LevelNodes.Find(Level);
        if (!NodesInLevel)
        {
            continue;
        }

        const int32 NodeCount = NodesInLevel->Num();
        const float TotalWidth = (NodeCount - 1) * NodeSpacingX;
        float LevelStartX = StartX + (MaxLevel > 0 ? 200.0f : 0.0f) - TotalWidth / 2.0f;
        LevelStartX = FMath::Max(LevelStartX, StartX);
        const float Y = StartY + Level * NodeSpacingY;

        for (int32 Index = 0; Index < NodeCount; ++Index)
        {
            Result.Add((*NodesInLevel)[Index], FVector2D(LevelStartX + Index * NodeSpacingX, Y));
        }
    }

    return Result;
}
