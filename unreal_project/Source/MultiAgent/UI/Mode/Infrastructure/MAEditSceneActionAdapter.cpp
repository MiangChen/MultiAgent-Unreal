#include "MAEditSceneActionAdapter.h"

#include "../../../Core/Editing/Runtime/MAEditModeManager.h"
#include "../../../Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "../../../Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.h"
#include "../../../Core/SceneGraph/Feedback/MASceneGraphFeedback.h"
#include "../../../Core/SceneGraph/Infrastructure/UE/MAUESceneApplier.h"
#include "../../../Environment/Utils/MAGoalActor.h"
#include "../../../Environment/Utils/MAPointOfInterest.h"
#include "../../../Environment/Utils/MAZoneActor.h"
#include "Engine/GameInstance.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    FMASceneActionResult ToSceneActionResult(const FMASceneGraphMutationFeedback& Feedback)
    {
        FMASceneActionResult Result;
        Result.bSuccess = Feedback.bSuccess;
        Result.bIsWarning = Feedback.Kind == EMASceneGraphFeedbackKind::Warning;
        Result.bClearSelection = Feedback.bClearSelection;
        Result.bRefreshSceneList = Feedback.bRefreshSceneList;
        Result.bRefreshSelection = Feedback.bRefreshSelection;
        Result.bReloadSceneVisualization = Feedback.bReloadSceneVisualization;
        Result.bClearHighlightedActor = Feedback.bClearHighlightedActor;
        Result.Message = Feedback.Message;
        return Result;
    }

    FMASceneActionResult MakeFailure(const FString& Message)
    {
        return ToSceneActionResult(MASceneGraphFeedback::MakeMutationFailure(Message));
    }

    FMASceneActionResult MakeSuccess(
        const FString& Message,
        bool bClearSelection = false,
        bool bRefreshSceneList = false,
        bool bRefreshSelection = false,
        bool bReloadSceneVisualization = false)
    {
        return ToSceneActionResult(
            MASceneGraphFeedback::MakeMutationSuccess(
                Message,
                bClearSelection,
                bRefreshSceneList,
                bRefreshSelection,
                bReloadSceneVisualization));
    }

    bool ResolveManagers(
        UWorld* World,
        UMAEditModeManager*& OutEditModeManager,
        UMASceneGraphManager*& OutSceneGraphManager,
        FString& OutError)
    {
        OutEditModeManager = nullptr;
        OutSceneGraphManager = nullptr;

        if (!World)
        {
            OutError = TEXT("Unable to get World");
            return false;
        }

        OutEditModeManager = World->GetSubsystem<UMAEditModeManager>();
        if (!OutEditModeManager)
        {
            OutError = TEXT("EditModeManager not found");
            return false;
        }

        OutSceneGraphManager = FMASceneGraphBootstrap::Resolve(World);
        if (!OutSceneGraphManager)
        {
            OutError = TEXT("SceneGraphManager not found");
            return false;
        }

        return true;
    }
}

FMASceneActionResult FMAEditSceneActionAdapter::ApplyNodeEdit(UWorld* World, AActor* Actor, const FString& JsonContent) const
{
    if (!Actor)
    {
        return MakeFailure(TEXT("Please select an Actor first"));
    }

    UMAEditModeManager* EditModeManager = nullptr;
    UMASceneGraphManager* SceneGraphManager = nullptr;
    FString ErrorMessage;
    if (!ResolveManagers(World, EditModeManager, SceneGraphManager, ErrorMessage))
    {
        return MakeFailure(FString::Printf(TEXT("Edit failed: %s"), *ErrorMessage));
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return MakeFailure(TEXT("Invalid JSON format"));
    }

    FString NodeId;
    if (!JsonObject->TryGetStringField(TEXT("id"), NodeId))
    {
        return MakeFailure(TEXT("Missing id field in JSON"));
    }

    FString OutError;
    if (!SceneGraphManager->EditNode(NodeId, JsonContent, OutError))
    {
        return MakeFailure(OutError);
    }

    const FMASceneGraphNode UpdatedNode = SceneGraphManager->GetNodeById(NodeId);
    if (UpdatedNode.IsValid())
    {
        FMAUESceneApplier::ApplyNodeToScene(World, UpdatedNode);
    }

    SceneGraphManager->SaveToSource();
    EditModeManager->ClearSelection();
    return MakeSuccess(TEXT("Changes saved"), true, false, false, true);
}

FMASceneActionResult FMAEditSceneActionAdapter::DeleteActor(UWorld* World, AActor* Actor) const
{
    if (!Actor)
    {
        return MakeFailure(TEXT("Please select an Actor first"));
    }

    UMAEditModeManager* EditModeManager = nullptr;
    UMASceneGraphManager* SceneGraphManager = nullptr;
    FString ErrorMessage;
    if (!ResolveManagers(World, EditModeManager, SceneGraphManager, ErrorMessage))
    {
        return MakeFailure(FString::Printf(TEXT("Delete failed: %s"), *ErrorMessage));
    }

    if (const AMAGoalActor* GoalActor = Cast<AMAGoalActor>(Actor))
    {
        const FString NodeId = GoalActor->GetNodeId();
        if (NodeId.IsEmpty())
        {
            return MakeFailure(TEXT("Delete failed: Unable to get Node ID"));
        }

        FString OutError;
        if (!SceneGraphManager->DeleteNode(NodeId, OutError))
        {
            return MakeFailure(OutError);
        }

        SceneGraphManager->SaveToSource();
        EditModeManager->DestroyGoalActor(NodeId);
        EditModeManager->ClearSelection();
        return MakeSuccess(TEXT("Deleted"), true, true, false, true);
    }

    if (const AMAZoneActor* ZoneActor = Cast<AMAZoneActor>(Actor))
    {
        const FString NodeId = ZoneActor->GetNodeId();
        if (NodeId.IsEmpty())
        {
            return MakeFailure(TEXT("Delete failed: Unable to get Node ID"));
        }

        FString OutError;
        if (!SceneGraphManager->DeleteNode(NodeId, OutError))
        {
            return MakeFailure(OutError);
        }

        SceneGraphManager->SaveToSource();
        EditModeManager->DestroyZoneActor(NodeId);
        EditModeManager->ClearSelection();
        return MakeSuccess(TEXT("Deleted"), true, true, false, true);
    }

    TArray<FMASceneGraphNode> MatchingNodes;
    FString ResolveError;
    if (!SceneGraphAdapter.ResolveActorNodes(World, Actor, MatchingNodes, ResolveError) || MatchingNodes.Num() == 0)
    {
        return MakeFailure(ResolveError.IsEmpty() ? TEXT("No matching scene graph node found") : ResolveError);
    }

    FString OutError;
    if (!SceneGraphManager->DeleteNode(MatchingNodes[0].Id, OutError))
    {
        return MakeFailure(OutError);
    }

    SceneGraphManager->SaveToSource();
    Actor->Destroy();
    EditModeManager->ClearSelection();
    return MakeSuccess(TEXT("Actor deleted"), true, false, false, true);
}

FMASceneActionResult FMAEditSceneActionAdapter::CreateGoal(UWorld* World, AMAPointOfInterest* POI, const FString& Description) const
{
    if (!POI)
    {
        return MakeFailure(TEXT("Please select a POI first"));
    }

    UMAEditModeManager* EditModeManager = nullptr;
    UMASceneGraphManager* SceneGraphManager = nullptr;
    FString ErrorMessage;
    if (!ResolveManagers(World, EditModeManager, SceneGraphManager, ErrorMessage))
    {
        return MakeFailure(FString::Printf(TEXT("Create failed: %s"), *ErrorMessage));
    }

    FString OutError;
    if (!EditModeManager->CreateGoal(POI->GetActorLocation(), Description, OutError))
    {
        return MakeFailure(OutError);
    }

    EditModeManager->DestroyPOI(POI);
    EditModeManager->ClearSelection();
    return MakeSuccess(TEXT("Goal created"), true, true, false, true);
}

FMASceneActionResult FMAEditSceneActionAdapter::CreateZone(UWorld* World, const TArray<AMAPointOfInterest*>& POIs, const FString& Description) const
{
    if (POIs.Num() < 3)
    {
        return MakeFailure(TEXT("Creating a zone requires at least 3 POIs"));
    }

    UMAEditModeManager* EditModeManager = nullptr;
    UMASceneGraphManager* SceneGraphManager = nullptr;
    FString ErrorMessage;
    if (!ResolveManagers(World, EditModeManager, SceneGraphManager, ErrorMessage))
    {
        return MakeFailure(FString::Printf(TEXT("Create failed: %s"), *ErrorMessage));
    }

    TArray<FVector> Vertices;
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            Vertices.Add(POI->GetActorLocation());
        }
    }

    if (Vertices.Num() < 3)
    {
        return MakeFailure(TEXT("Insufficient valid POIs"));
    }

    FString OutError;
    if (!EditModeManager->CreateZone(Vertices, Description, OutError))
    {
        return MakeFailure(OutError);
    }

    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            EditModeManager->DestroyPOI(POI);
        }
    }

    EditModeManager->ClearSelection();
    return MakeSuccess(TEXT("Zone created"), true, true, false, true);
}

FMASceneActionResult FMAEditSceneActionAdapter::AddPresetActor(UWorld* World, AMAPointOfInterest* POI, const FString& ActorType) const
{
    if (!POI)
    {
        return MakeFailure(TEXT("Please select a POI first"));
    }

    if (ActorType.IsEmpty() || ActorType == TEXT("(No preset Actors)"))
    {
        return MakeFailure(TEXT("Please select a valid preset Actor type"));
    }

    if (!World)
    {
        return MakeFailure(TEXT("Add failed: Unable to get World"));
    }

    FString ErrorMessage;
    UMAEditModeManager* EditModeManager = nullptr;
    UMASceneGraphManager* SceneGraphManager = nullptr;
    if (!ResolveManagers(World, EditModeManager, SceneGraphManager, ErrorMessage))
    {
        return MakeFailure(FString::Printf(TEXT("Add failed: %s"), *ErrorMessage));
    }

    FMASceneActionResult Result = MakeFailure(TEXT("Preset Actor feature not yet implemented"));
    Result.bIsWarning = true;
    return Result;
}

FMASceneActionResult FMAEditSceneActionAdapter::DeletePOIs(UWorld* World, const TArray<AMAPointOfInterest*>& POIs) const
{
    if (POIs.Num() == 0)
    {
        return MakeFailure(TEXT("Please select POI first"));
    }

    FString ErrorMessage;
    UMAEditModeManager* EditModeManager = nullptr;
    UMASceneGraphManager* SceneGraphManager = nullptr;
    if (!ResolveManagers(World, EditModeManager, SceneGraphManager, ErrorMessage))
    {
        return MakeFailure(FString::Printf(TEXT("Delete failed: %s"), *ErrorMessage));
    }

    int32 DeletedCount = 0;
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            EditModeManager->DestroyPOI(POI);
            DeletedCount++;
        }
    }

    return MakeSuccess(FString::Printf(TEXT("Deleted %d POIs"), DeletedCount), false, false, false, false);
}

FMASceneActionResult FMAEditSceneActionAdapter::SetGoalState(UWorld* World, AActor* Actor, bool bShouldSetGoal) const
{
    if (!Actor)
    {
        return MakeFailure(TEXT("Please select an Actor first"));
    }

    FString ErrorMessage;
    UMAEditModeManager* EditModeManager = nullptr;
    UMASceneGraphManager* SceneGraphManager = nullptr;
    if (!ResolveManagers(World, EditModeManager, SceneGraphManager, ErrorMessage))
    {
        const FString Prefix = bShouldSetGoal ? TEXT("Set failed: ") : TEXT("Unset failed: ");
        return MakeFailure(Prefix + ErrorMessage);
    }

    TArray<FMASceneGraphNode> MatchingNodes;
    FString ResolveError;
    if (!SceneGraphAdapter.ResolveActorNodes(World, Actor, MatchingNodes, ResolveError) || MatchingNodes.Num() == 0)
    {
        return MakeFailure(ResolveError.IsEmpty() ? TEXT("No matching scene graph node found") : ResolveError);
    }

    FString OutError;
    const bool bSucceeded = bShouldSetGoal
        ? SceneGraphManager->SetNodeAsGoal(MatchingNodes[0].Id, OutError)
        : SceneGraphManager->UnsetNodeAsGoal(MatchingNodes[0].Id, OutError);
    if (!bSucceeded)
    {
        return MakeFailure(OutError);
    }

    FString NodeLabel = MatchingNodes[0].Label;
    if (NodeLabel.IsEmpty())
    {
        NodeLabel = MatchingNodes[0].Id;
    }

    const FString Message = bShouldSetGoal
        ? FString::Printf(TEXT("Set %s as Goal"), *NodeLabel)
        : FString::Printf(TEXT("Unset %s as Goal"), *NodeLabel);

    return MakeSuccess(Message, false, true, true, false);
}
