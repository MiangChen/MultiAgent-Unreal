#pragma once

#include "CoreMinimal.h"
#include "Core/SceneGraph/Feedback/MASceneGraphFeedback.h"

class UGameInstance;
class UMASceneGraphManager;
class UObject;
class UWorld;

struct MULTIAGENT_API FMASceneGraphBootstrap
{
    static constexpr const TCHAR* ContextName = TEXT("SceneGraph");

    static UWorld* ResolveWorld(const UObject* WorldContextObject);
    static UGameInstance* ResolveGameInstance(const UObject* WorldContextObject);
    static UMASceneGraphManager* Resolve(const UObject* WorldContextObject);
    static bool TryResolve(
        const UObject* WorldContextObject,
        UMASceneGraphManager*& OutManager,
        FString& OutError);
    static FMASceneGraphNodesFeedback LoadAllNodes(const UObject* WorldContextObject);
    static FMASceneGraphNodesFeedback LoadGoals(const UObject* WorldContextObject);
    static FMASceneGraphNodesFeedback LoadZones(const UObject* WorldContextObject);
    static FMASceneGraphNodeFeedback LoadNodeById(
        const UObject* WorldContextObject,
        const FString& NodeId);
};
