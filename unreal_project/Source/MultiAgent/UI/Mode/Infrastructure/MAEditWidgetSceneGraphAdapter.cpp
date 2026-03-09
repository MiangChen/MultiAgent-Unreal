#include "MAEditWidgetSceneGraphAdapter.h"

#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Environment/Utils/MAGoalActor.h"
#include "../../../Environment/Utils/MAZoneActor.h"
#include "Dom/JsonObject.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    FString ResolveActorSearchId(const AActor* Actor, bool& bOutIsGoalOrZone)
    {
        bOutIsGoalOrZone = false;

        if (!Actor)
        {
            return FString();
        }

        if (const AMAGoalActor* GoalActor = Cast<AMAGoalActor>(Actor))
        {
            bOutIsGoalOrZone = true;
            return GoalActor->GetNodeId();
        }

        if (const AMAZoneActor* ZoneActor = Cast<AMAZoneActor>(Actor))
        {
            bOutIsGoalOrZone = true;
            return ZoneActor->GetNodeId();
        }

        return Actor->GetActorGuid().ToString();
    }
}

bool FMAEditWidgetSceneGraphAdapter::ResolveActorNodes(
    UWorld* World,
    AActor* Actor,
    TArray<FMASceneGraphNode>& OutNodes,
    FString& OutError) const
{
    OutNodes.Empty();
    OutError.Empty();

    if (!World || !Actor)
    {
        OutError = TEXT("Actor selection is invalid");
        return false;
    }

    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(World);
    UMASceneGraphManager* SceneGraphManager = GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    if (!SceneGraphManager)
    {
        OutError = TEXT("SceneGraphManager not found");
        return false;
    }

    bool bIsGoalOrZone = false;
    const FString SearchId = ResolveActorSearchId(Actor, bIsGoalOrZone);
    if (SearchId.IsEmpty())
    {
        OutError = TEXT("Unable to resolve actor node id");
        return false;
    }

    if (bIsGoalOrZone)
    {
        const FMASceneGraphNode Node = SceneGraphManager->GetNodeById(SearchId);
        if (Node.IsValid())
        {
            OutNodes.Add(Node);
            return true;
        }

        OutError = FString::Printf(TEXT("Node not found: %s"), *SearchId);
        return false;
    }

    OutNodes = SceneGraphManager->FindNodesByGuid(SearchId);
    if (OutNodes.Num() == 0)
    {
        OutError = TEXT("No matching scene graph node found");
        return false;
    }

    return true;
}

bool FMAEditWidgetSceneGraphAdapter::ValidateJsonDocument(const FString& Json, FString& OutError) const
{
    if (Json.IsEmpty())
    {
        OutError = TEXT("JSON content cannot be empty");
        return false;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        OutError = TEXT("Invalid JSON format, please check syntax");
        return false;
    }

    if (!JsonObject->HasField(TEXT("id")))
    {
        OutError = TEXT("Missing required field: id");
        return false;
    }

    OutError.Empty();
    return true;
}

FString FMAEditWidgetSceneGraphAdapter::BuildEditableJson(const FMASceneGraphNode& Node) const
{
    if (!Node.RawJson.IsEmpty())
    {
        return Node.RawJson;
    }

    return FString::Printf(
        TEXT("{\n  \"id\": \"%s\",\n  \"type\": \"%s\",\n  \"label\": \"%s\",\n  \"shape\": {\n    \"type\": \"%s\",\n    \"center\": [%.0f, %.0f, %.0f]\n  }\n}"),
        *Node.Id,
        *Node.Type,
        *Node.Label,
        *Node.ShapeType,
        Node.Center.X, Node.Center.Y, Node.Center.Z);
}

bool FMAEditWidgetSceneGraphAdapter::IsPointTypeNode(const FMASceneGraphNode& Node) const
{
    return Node.ShapeType.Equals(TEXT("point"), ESearchCase::IgnoreCase);
}

bool FMAEditWidgetSceneGraphAdapter::IsGoalOrZoneActor(const AActor* Actor) const
{
    return Actor && (Actor->IsA<AMAGoalActor>() || Actor->IsA<AMAZoneActor>());
}

bool FMAEditWidgetSceneGraphAdapter::IsNodeMarkedGoal(const FMASceneGraphNode& Node) const
{
    return Node.RawJson.Contains(TEXT("\"is_goal\"")) &&
        (Node.RawJson.Contains(TEXT("\"is_goal\": true")) ||
         Node.RawJson.Contains(TEXT("\"is_goal\":true")));
}
