#include "MATaskPlannerCoordinator.h"
#include "UI/TaskGraph/MATaskGraphModel.h"
#include "Core/TaskGraph/Application/MATaskGraphUseCases.h"
#include "Core/TaskGraph/Bootstrap/MATaskGraphBootstrap.h"

namespace
{
FMATaskPlannerActionResult MakeFailedResult(const FString& Message)
{
    FMATaskPlannerActionResult Result;
    Result.Message = Message;
    return Result;
}

FMATaskPlannerActionResult MakeGraphChangedResult(const FString& Message, const FMATaskGraphData& Data, bool bShouldPersist = true)
{
    FMATaskPlannerActionResult Result;
    Result.bSuccess = true;
    Result.bGraphChanged = true;
    Result.bShouldPersist = bShouldPersist;
    Result.bHasData = true;
    Result.Message = Message;
    Result.Data = Data;
    return Result;
}
}

FMATaskPlannerActionResult FMATaskPlannerCoordinator::LoadGraphData(UMATaskGraphModel* GraphModel, const FMATaskGraphData& Data, const FString& SuccessMessage)
{
    if (!GraphModel)
    {
        return MakeFailedResult(TEXT("[Error] Task graph model not initialized"));
    }

    GraphModel->LoadFromData(Data);
    return MakeGraphChangedResult(SuccessMessage, GraphModel->GetWorkingData(), false);
}

FMATaskPlannerActionResult FMATaskPlannerCoordinator::LoadGraphJson(
    UMATaskGraphModel* GraphModel,
    const FString& JsonText,
    const FString& EmptyMessage,
    const FString& SuccessPrefix)
{
    if (!GraphModel)
    {
        return MakeFailedResult(TEXT("[Error] Task graph model not initialized"));
    }

    if (JsonText.TrimStartAndEnd().IsEmpty())
    {
        return MakeFailedResult(EmptyMessage);
    }

    FString ErrorMessage;
    if (!GraphModel->LoadFromJsonWithError(JsonText, ErrorMessage))
    {
        return MakeFailedResult(FString::Printf(TEXT("[Error] JSON parse failed: %s"), *ErrorMessage));
    }

    const FMATaskGraphData Data = GraphModel->GetWorkingData();
    return MakeGraphChangedResult(
        FString::Printf(TEXT("%s%d nodes, %d edges"), *SuccessPrefix, Data.Nodes.Num(), Data.Edges.Num()),
        Data);
}

FMATaskPlannerActionResult FMATaskPlannerCoordinator::LoadMockData(UMATaskGraphModel* GraphModel, const FString& ProjectDir)
{
    if (!GraphModel)
    {
        return MakeFailedResult(TEXT("[Error] Task graph model not initialized"));
    }

    const FString FilePath = FTaskGraphBootstrap::GetMockResponseExamplePath(ProjectDir);
    const FTaskGraphLoadResult LoadResult = FTaskGraphUseCases::LoadResponseExampleFile(FilePath);
    if (!LoadResult.bSuccess)
    {
        return MakeFailedResult(FString::Printf(TEXT("[Error] %s"), *LoadResult.Feedback.Message));
    }

    GraphModel->LoadFromData(LoadResult.Data);
    return MakeGraphChangedResult(TEXT("[Success] Mock data loaded"), GraphModel->GetWorkingData(), false);
}

FMATaskPlannerActionResult FMATaskPlannerCoordinator::AddNodeFromTemplate(UMATaskGraphModel* GraphModel, const FMANodeTemplate& Template)
{
    if (!GraphModel)
    {
        return MakeFailedResult(TEXT("[Error] Task graph model not initialized"));
    }

    const FString NewNodeId = GraphModel->CreateNode(Template.DefaultDescription, Template.DefaultLocation);
    return MakeGraphChangedResult(
        FString::Printf(TEXT("Node created: %s (%s)"), *NewNodeId, *Template.TemplateName),
        GraphModel->GetWorkingData());
}

FMATaskPlannerActionResult FMATaskPlannerCoordinator::DeleteNode(UMATaskGraphModel* GraphModel, const FString& NodeId)
{
    if (!GraphModel)
    {
        return MakeFailedResult(TEXT("[Error] Task graph model not initialized"));
    }

    if (!GraphModel->RemoveNode(NodeId))
    {
        return MakeFailedResult(FString::Printf(TEXT("[Error] Node not found: %s"), *NodeId));
    }

    return MakeGraphChangedResult(FString::Printf(TEXT("Node deleted: %s"), *NodeId), GraphModel->GetWorkingData());
}

FMATaskPlannerActionResult FMATaskPlannerCoordinator::AddEdge(UMATaskGraphModel* GraphModel, const FString& FromNodeId, const FString& ToNodeId)
{
    if (!GraphModel)
    {
        return MakeFailedResult(TEXT("[Error] Task graph model not initialized"));
    }

    if (!GraphModel->AddEdge(FromNodeId, ToNodeId))
    {
        return MakeFailedResult(FString::Printf(TEXT("[Error] Failed to create edge: %s -> %s"), *FromNodeId, *ToNodeId));
    }

    return MakeGraphChangedResult(
        FString::Printf(TEXT("Edge created: %s -> %s"), *FromNodeId, *ToNodeId),
        GraphModel->GetWorkingData());
}

FMATaskPlannerActionResult FMATaskPlannerCoordinator::DeleteEdge(UMATaskGraphModel* GraphModel, const FString& FromNodeId, const FString& ToNodeId)
{
    if (!GraphModel)
    {
        return MakeFailedResult(TEXT("[Error] Task graph model not initialized"));
    }

    if (!GraphModel->RemoveEdge(FromNodeId, ToNodeId))
    {
        return MakeFailedResult(FString::Printf(TEXT("[Error] Edge not found: %s -> %s"), *FromNodeId, *ToNodeId));
    }

    return MakeGraphChangedResult(
        FString::Printf(TEXT("Edge deleted: %s -> %s"), *FromNodeId, *ToNodeId),
        GraphModel->GetWorkingData());
}

FMATaskPlannerActionResult FMATaskPlannerCoordinator::DescribeNode(UMATaskGraphModel* GraphModel, const FString& NodeId)
{
    if (!GraphModel)
    {
        return MakeFailedResult(TEXT("[Error] Task graph model not initialized"));
    }

    FMATaskNodeData NodeData;
    if (!GraphModel->FindNode(NodeId, NodeData))
    {
        return MakeFailedResult(FString::Printf(TEXT("[Error] Node not found: %s"), *NodeId));
    }

    FMATaskPlannerActionResult Result;
    Result.bSuccess = true;
    Result.Message = FString::Printf(TEXT("Edit node: %s"), *NodeId);
    Result.DetailLines = {
        FString::Printf(TEXT("  Description: %s"), *NodeData.Description),
        FString::Printf(TEXT("  Location: %s"), *NodeData.Location)
    };
    return Result;
}
