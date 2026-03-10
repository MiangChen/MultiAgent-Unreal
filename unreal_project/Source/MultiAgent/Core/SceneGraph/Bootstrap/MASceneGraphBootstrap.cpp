#include "Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.h"

#include "Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

namespace
{
template <typename Getter>
FMASceneGraphNodesFeedback LoadNodesWithResolver(
    const UObject* WorldContextObject,
    Getter&& GetNodes)
{
    UMASceneGraphManager* SceneGraphManager = nullptr;
    FString Error;
    if (!FMASceneGraphBootstrap::TryResolve(WorldContextObject, SceneGraphManager, Error))
    {
        return MASceneGraphFeedback::MakeNodesFailure(Error);
    }

    return MASceneGraphFeedback::MakeNodesSuccess(GetNodes(*SceneGraphManager));
}
}

UWorld* FMASceneGraphBootstrap::ResolveWorld(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (const UWorld* DirectWorld = Cast<UWorld>(WorldContextObject))
    {
        return const_cast<UWorld*>(DirectWorld);
    }

    return WorldContextObject->GetWorld();
}

UGameInstance* FMASceneGraphBootstrap::ResolveGameInstance(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (const UGameInstance* DirectGameInstance = Cast<UGameInstance>(WorldContextObject))
    {
        return const_cast<UGameInstance*>(DirectGameInstance);
    }

    if (UWorld* World = ResolveWorld(WorldContextObject))
    {
        return World->GetGameInstance();
    }

    return nullptr;
}

UMASceneGraphManager* FMASceneGraphBootstrap::Resolve(const UObject* WorldContextObject)
{
    if (UGameInstance* GameInstance = ResolveGameInstance(WorldContextObject))
    {
        return GameInstance->GetSubsystem<UMASceneGraphManager>();
    }

    return nullptr;
}

bool FMASceneGraphBootstrap::TryResolve(
    const UObject* WorldContextObject,
    UMASceneGraphManager*& OutManager,
    FString& OutError)
{
    OutManager = nullptr;
    OutError.Empty();

    if (!WorldContextObject)
    {
        OutError = TEXT("SceneGraph unavailable: world context is null");
        return false;
    }

    UGameInstance* GameInstance = ResolveGameInstance(WorldContextObject);
    if (!GameInstance)
    {
        OutError = TEXT("SceneGraph unavailable: game instance not found");
        return false;
    }

    OutManager = GameInstance->GetSubsystem<UMASceneGraphManager>();
    if (!OutManager)
    {
        OutError = TEXT("SceneGraph unavailable: manager not found");
        return false;
    }

    return true;
}

FMASceneGraphNodesFeedback FMASceneGraphBootstrap::LoadAllNodes(const UObject* WorldContextObject)
{
    return LoadNodesWithResolver(
        WorldContextObject,
        [](UMASceneGraphManager& Manager)
        {
            return Manager.GetAllNodes();
        });
}

FMASceneGraphNodesFeedback FMASceneGraphBootstrap::LoadGoals(const UObject* WorldContextObject)
{
    return LoadNodesWithResolver(
        WorldContextObject,
        [](UMASceneGraphManager& Manager)
        {
            return Manager.GetAllGoals();
        });
}

FMASceneGraphNodesFeedback FMASceneGraphBootstrap::LoadZones(const UObject* WorldContextObject)
{
    return LoadNodesWithResolver(
        WorldContextObject,
        [](UMASceneGraphManager& Manager)
        {
            return Manager.GetAllZones();
        });
}

FMASceneGraphNodeFeedback FMASceneGraphBootstrap::LoadNodeById(
    const UObject* WorldContextObject,
    const FString& NodeId)
{
    UMASceneGraphManager* SceneGraphManager = nullptr;
    FString Error;
    if (!TryResolve(WorldContextObject, SceneGraphManager, Error))
    {
        return MASceneGraphFeedback::MakeNodeFailure(Error);
    }

    const FMASceneGraphNode Node = SceneGraphManager->GetNodeById(NodeId);
    if (!Node.IsValid())
    {
        return MASceneGraphFeedback::MakeNodeFailure(
            FString::Printf(TEXT("Scene graph node not found: %s"), *NodeId));
    }

    return MASceneGraphFeedback::MakeNodeSuccess(Node);
}
