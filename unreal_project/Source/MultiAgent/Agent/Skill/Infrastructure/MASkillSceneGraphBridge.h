#pragma once

#include "CoreMinimal.h"
#include "Core/SceneGraph/Domain/MASceneGraphNodeTypes.h"

class UObject;
class UMASceneGraphManager;

struct MULTIAGENT_API FMASkillSceneGraphBridge
{
    static UMASceneGraphManager* ResolveManager(const UObject* WorldContextObject);
    static TArray<FMASceneGraphNode> LoadAllNodes(const UObject* WorldContextObject);
    static TArray<FMASceneGraphNode> LoadAllBuildings(const UObject* WorldContextObject);
    static FMASceneGraphNode LoadNodeById(const UObject* WorldContextObject, const FString& NodeId);
    static bool ContainsNodeId(const UObject* WorldContextObject, const FString& NodeId);
    static FMASceneGraphNode FindNearestLandmark(const UObject* WorldContextObject, const FVector& Location, float MaxDistance);
    static bool IsPointInsideBuilding(const UObject* WorldContextObject, const FVector& Point);
};
