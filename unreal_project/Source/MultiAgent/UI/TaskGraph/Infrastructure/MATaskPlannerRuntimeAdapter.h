#pragma once

#include "CoreMinimal.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"

class UObject;
class UMATaskPlannerWidget;

class FMATaskPlannerRuntimeAdapter
{
public:
    static bool TryLoadTaskGraph(const UObject* WorldContextObject, FMATaskGraphData& OutData, FString& OutError);
    static bool TrySaveTaskGraph(const UObject* WorldContextObject, const FMATaskGraphData& Data, FString& OutError);
    static bool TrySubmitTaskGraph(const UObject* WorldContextObject, const FString& TaskGraphJson, FString& OutError);
    static bool TryNavigateToTaskGraphModal(const UObject* WorldContextObject, FString& OutError);
    static bool TryHideTaskPlanner(const UObject* WorldContextObject, FString& OutError);
    static bool BindTaskGraphChanged(const UObject* WorldContextObject, UMATaskPlannerWidget* Widget, FString& OutError);
};
