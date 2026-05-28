#pragma once

#include "CoreMinimal.h"
#include "../Feedback/MATaskPlannerFeedback.h"

class UMATaskGraphModel;

class FMATaskPlannerCoordinator
{
public:
    static FMATaskPlannerActionResult LoadGraphData(UMATaskGraphModel* GraphModel, const FMATaskGraphData& Data, const FString& SuccessMessage);
    static FMATaskPlannerActionResult LoadGraphJson(UMATaskGraphModel* GraphModel, const FString& JsonText, const FString& EmptyMessage, const FString& SuccessPrefix);
    static FMATaskPlannerActionResult LoadMockData(UMATaskGraphModel* GraphModel, const FString& ProjectDir);
    static FMATaskPlannerActionResult AddNodeFromTemplate(UMATaskGraphModel* GraphModel, const FMANodeTemplate& Template);
    static FMATaskPlannerActionResult DeleteNode(UMATaskGraphModel* GraphModel, const FString& NodeId);
    static FMATaskPlannerActionResult AddEdge(UMATaskGraphModel* GraphModel, const FString& FromNodeId, const FString& ToNodeId);
    static FMATaskPlannerActionResult DeleteEdge(UMATaskGraphModel* GraphModel, const FString& FromNodeId, const FString& ToNodeId);
    static FMATaskPlannerActionResult DescribeNode(UMATaskGraphModel* GraphModel, const FString& NodeId);
};
