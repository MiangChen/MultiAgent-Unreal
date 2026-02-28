// MASceneGraphQueryUseCases.cpp

#include "scene_graph_services/MASceneGraphQueryUseCases.h"
#include "scene_graph_tools/MASceneGraphQuery.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::BuildAllNodes(
    const TArray<FMASceneGraphNode>* StaticNodes,
    const TArray<FMASceneGraphNode>* DynamicNodes)
{
    if (!StaticNodes || !DynamicNodes)
    {
        return {};
    }
    return FMASceneGraphQuery::GetAllNodes(*StaticNodes, *DynamicNodes);
}

FMASceneGraphNode FMASceneGraphQueryUseCases::GetNodeById(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& NodeId)
{
    return FMASceneGraphQuery::GetNodeById(AllNodes, NodeId);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::FindNodesByGuid(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& ActorGuid)
{
    return FMASceneGraphQuery::FindNodesByGuid(AllNodes, ActorGuid);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::FindAllNodesByLabel(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FMASemanticLabel& Label)
{
    return FMASceneGraphQuery::FindAllNodesByLabel(AllNodes, Label);
}

FMASceneGraphNode FMASceneGraphQueryUseCases::FindNodeByIdOrLabel(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& NodeIdOrLabel)
{
    return FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, NodeIdOrLabel);
}

FString FMASceneGraphQueryUseCases::GetIdByLabel(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& Label)
{
    return FMASceneGraphQuery::GetIdByLabel(AllNodes, Label);
}

FString FMASceneGraphQueryUseCases::GetLabelById(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& NodeId)
{
    return FMASceneGraphQuery::GetLabelById(AllNodes, NodeId);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::GetAllBuildings(const TArray<FMASceneGraphNode>& AllNodes)
{
    return FMASceneGraphQuery::GetAllBuildings(AllNodes);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::GetAllRoads(const TArray<FMASceneGraphNode>& AllNodes)
{
    return FMASceneGraphQuery::GetAllRoads(AllNodes);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::GetAllIntersections(const TArray<FMASceneGraphNode>& AllNodes)
{
    return FMASceneGraphQuery::GetAllIntersections(AllNodes);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::GetAllProps(const TArray<FMASceneGraphNode>& AllNodes)
{
    return FMASceneGraphQuery::GetAllProps(AllNodes);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::GetAllRobots(const TArray<FMASceneGraphNode>& AllNodes)
{
    return FMASceneGraphQuery::GetAllRobots(AllNodes);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::GetAllPickupItems(const TArray<FMASceneGraphNode>& AllNodes)
{
    return FMASceneGraphQuery::GetAllPickupItems(AllNodes);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::GetAllChargingStations(const TArray<FMASceneGraphNode>& AllNodes)
{
    return FMASceneGraphQuery::GetAllChargingStations(AllNodes);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::GetAllGoals(const TArray<FMASceneGraphNode>& AllNodes)
{
    return FMASceneGraphQuery::GetAllGoals(AllNodes);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::GetAllZones(const TArray<FMASceneGraphNode>& AllNodes)
{
    return FMASceneGraphQuery::GetAllZones(AllNodes);
}

FMASceneGraphNode FMASceneGraphQueryUseCases::FindNodeByLabel(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FMASemanticLabel& Label)
{
    return FMASceneGraphQuery::FindNodeByLabel(AllNodes, Label);
}

FMASceneGraphNode FMASceneGraphQueryUseCases::FindNearestNode(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FMASemanticLabel& Label,
    const FVector& FromLocation)
{
    return FMASceneGraphQuery::FindNearestNode(AllNodes, Label, FromLocation);
}

TArray<FMASceneGraphNode> FMASceneGraphQueryUseCases::FindNodesInBoundary(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FMASemanticLabel& Label,
    const TArray<FVector>& BoundaryVertices)
{
    return FMASceneGraphQuery::FindNodesInBoundary(AllNodes, Label, BoundaryVertices);
}

bool FMASceneGraphQueryUseCases::IsPointInsideBuilding(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FVector& Point)
{
    return FMASceneGraphQuery::IsPointInsideBuilding(AllNodes, Point);
}

FMASceneGraphNode FMASceneGraphQueryUseCases::FindNearestLandmark(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FVector& Location,
    float MaxDistance)
{
    return FMASceneGraphQuery::FindNearestLandmark(AllNodes, Location, MaxDistance);
}

TSharedPtr<FJsonObject> FMASceneGraphQueryUseCases::NodeToJsonObject(const FMASceneGraphNode& Node)
{
    return FMASceneGraphQuery::NodeToJsonObject(Node);
}

bool FMASceneGraphQueryUseCases::PassesFilter(
    const FMASceneGraphNode& Node,
    const FString& CategoryFilter,
    const FString& TypeFilter,
    const FString& LabelFilter)
{
    if (!CategoryFilter.IsEmpty() && !Node.Category.Equals(CategoryFilter, ESearchCase::IgnoreCase))
    {
        return false;
    }

    if (!TypeFilter.IsEmpty() && !Node.Type.Equals(TypeFilter, ESearchCase::IgnoreCase))
    {
        return false;
    }

    if (!LabelFilter.IsEmpty() && !Node.Label.Contains(LabelFilter, ESearchCase::IgnoreCase))
    {
        return false;
    }

    return true;
}

FString FMASceneGraphQueryUseCases::BuildWorldStateJson(
    const TArray<FMASceneGraphNode>& AllNodes,
    const FString& CategoryFilter,
    const FString& TypeFilter,
    const FString& LabelFilter)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    TArray<TSharedPtr<FJsonValue>> EdgesArray;

    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (!PassesFilter(Node, CategoryFilter, TypeFilter, LabelFilter))
        {
            continue;
        }

        TSharedPtr<FJsonObject> NodeJson = NodeToJsonObject(Node);
        NodesArray.Add(MakeShareable(new FJsonValueObject(NodeJson)));
    }

    RootObject->SetArrayField(TEXT("nodes"), NodesArray);
    RootObject->SetArrayField(TEXT("edges"), EdgesArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    return OutputString;
}
