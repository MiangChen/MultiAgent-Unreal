#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"

DEFINE_LOG_CATEGORY(LogMATaskGraph);

bool FMARequiredSkill::operator==(const FMARequiredSkill& Other) const
{
    return SkillName == Other.SkillName &&
           AssignedRobotType == Other.AssignedRobotType &&
           AssignedRobotCount == Other.AssignedRobotCount;
}

bool FMATaskNodeData::operator==(const FMATaskNodeData& Other) const
{
    return TaskId == Other.TaskId &&
           Description == Other.Description &&
           Location == Other.Location &&
           RequiredSkills == Other.RequiredSkills &&
           Produces == Other.Produces;
}

bool FMATaskEdgeData::operator==(const FMATaskEdgeData& Other) const
{
    return FromNodeId == Other.FromNodeId &&
           ToNodeId == Other.ToNodeId &&
           EdgeType == Other.EdgeType &&
           Condition == Other.Condition;
}

FMATaskNodeData* FMATaskGraphData::FindNode(const FString& TaskId)
{
    for (FMATaskNodeData& Node : Nodes)
    {
        if (Node.TaskId == TaskId)
        {
            return &Node;
        }
    }
    return nullptr;
}

const FMATaskNodeData* FMATaskGraphData::FindNode(const FString& TaskId) const
{
    for (const FMATaskNodeData& Node : Nodes)
    {
        if (Node.TaskId == TaskId)
        {
            return &Node;
        }
    }
    return nullptr;
}

bool FMATaskGraphData::HasNode(const FString& TaskId) const
{
    return FindNode(TaskId) != nullptr;
}

bool FMATaskGraphData::HasEdge(const FString& FromId, const FString& ToId) const
{
    for (const FMATaskEdgeData& Edge : Edges)
    {
        if (Edge.FromNodeId == FromId && Edge.ToNodeId == ToId)
        {
            return true;
        }
    }
    return false;
}

bool FMATaskGraphData::IsValidDAG() const
{
    TMap<FString, TArray<FString>> AdjList;
    TMap<FString, int32> InDegree;

    for (const FMATaskNodeData& Node : Nodes)
    {
        AdjList.Add(Node.TaskId, TArray<FString>());
        InDegree.Add(Node.TaskId, 0);
    }

    for (const FMATaskEdgeData& Edge : Edges)
    {
        if (!AdjList.Contains(Edge.FromNodeId) || !AdjList.Contains(Edge.ToNodeId))
        {
            return false;
        }
        AdjList[Edge.FromNodeId].Add(Edge.ToNodeId);
        InDegree[Edge.ToNodeId]++;
    }

    TArray<FString> Queue;
    for (const auto& Pair : InDegree)
    {
        if (Pair.Value == 0)
        {
            Queue.Add(Pair.Key);
        }
    }

    int32 ProcessedCount = 0;
    while (Queue.Num() > 0)
    {
        const FString Current = Queue[0];
        Queue.RemoveAt(0);
        ++ProcessedCount;

        for (const FString& Neighbor : AdjList[Current])
        {
            InDegree[Neighbor]--;
            if (InDegree[Neighbor] == 0)
            {
                Queue.Add(Neighbor);
            }
        }
    }

    return ProcessedCount == Nodes.Num();
}

bool FMATaskGraphData::operator==(const FMATaskGraphData& Other) const
{
    if (Description != Other.Description)
    {
        return false;
    }
    if (Nodes.Num() != Other.Nodes.Num() || Edges.Num() != Other.Edges.Num())
    {
        return false;
    }

    for (int32 Index = 0; Index < Nodes.Num(); ++Index)
    {
        if (!(Nodes[Index] == Other.Nodes[Index]))
        {
            return false;
        }
    }

    for (int32 Index = 0; Index < Edges.Num(); ++Index)
    {
        if (!(Edges[Index] == Other.Edges[Index]))
        {
            return false;
        }
    }

    return true;
}
