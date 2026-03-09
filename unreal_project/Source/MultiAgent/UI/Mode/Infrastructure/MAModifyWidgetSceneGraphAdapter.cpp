#include "MAModifyWidgetSceneGraphAdapter.h"

#include "../../../Core/Manager/MASceneGraphManager.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    FString FormatJsonPreviewWithHighlight(const FMASceneGraphNode& Node, const FString& ActorGuid)
    {
        FString Result;
        FString TypeTitle;
        if (Node.ShapeType == TEXT("polygon"))
        {
            TypeTitle = TEXT("=== Polygon Node ===");
        }
        else if (Node.ShapeType == TEXT("linestring"))
        {
            TypeTitle = TEXT("=== LineString Node ===");
        }
        else
        {
            TypeTitle = TEXT("=== Point Node ===");
        }

        Result += TypeTitle + TEXT("\n\n");
        Result += FString::Printf(TEXT("id: %s\n"), *Node.Id);
        Result += FString::Printf(TEXT("type: %s\n"), *Node.Type);
        Result += FString::Printf(TEXT("label: %s\n"), *Node.Label);
        Result += FString::Printf(TEXT("center: [%.0f, %.0f, %.0f]\n"), Node.Center.X, Node.Center.Y, Node.Center.Z);

        if (Node.GuidArray.Num() > 0)
        {
            Result += TEXT("\nGuid Array:\n");
            for (int32 Index = 0; Index < Node.GuidArray.Num(); ++Index)
            {
                const FString& Guid = Node.GuidArray[Index];
                if (Guid == ActorGuid)
                {
                    Result += FString::Printf(TEXT("  [%d] >>> %s <<< (selected)\n"), Index, *Guid);
                }
                else
                {
                    Result += FString::Printf(TEXT("  [%d] %s\n"), Index, *Guid);
                }
            }
        }
        else if (!Node.Guid.IsEmpty())
        {
            if (Node.Guid == ActorGuid)
            {
                Result += FString::Printf(TEXT("\nguid: >>> %s <<< (selected)\n"), *Node.Guid);
            }
            else
            {
                Result += FString::Printf(TEXT("\nguid: %s\n"), *Node.Guid);
            }
        }

        return Result;
    }

    UMASceneGraphManager* ResolveSceneGraphManager(UWorld* World)
    {
        UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
        return GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    }
}

bool FMAModifyWidgetSceneGraphAdapter::FindMatchingNodes(
    UWorld* World,
    const FString& ActorGuid,
    TArray<FMASceneGraphNode>& OutNodes,
    FString& OutError) const
{
    OutNodes.Empty();
    OutError.Empty();

    if (!World)
    {
        OutError = TEXT("Unable to get World");
        return false;
    }

    UMASceneGraphManager* SceneGraphManager = ResolveSceneGraphManager(World);
    if (!SceneGraphManager)
    {
        OutError = TEXT("SceneGraphManager not found");
        return false;
    }

    OutNodes = SceneGraphManager->FindNodesByGuid(ActorGuid);

    const TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
    for (const FMASceneGraphNode& Node : AllNodes)
    {
        if (Node.Guid == ActorGuid)
        {
            const bool bAlreadyAdded = OutNodes.ContainsByPredicate(
                [&Node](const FMASceneGraphNode& ExistingNode)
                {
                    return ExistingNode.Id == Node.Id;
                });
            if (!bAlreadyAdded)
            {
                OutNodes.Add(Node);
            }
        }
    }

    if (OutNodes.Num() == 0)
    {
        OutError = FString::Printf(TEXT("No node found for Actor GUID %s"), *ActorGuid);
        return false;
    }

    return true;
}

bool FMAModifyWidgetSceneGraphAdapter::FindSharedNodeForActors(
    UWorld* World,
    const TArray<AActor*>& Actors,
    FMASceneGraphNode& OutNode,
    FString& OutError) const
{
    OutNode = FMASceneGraphNode();
    OutError.Empty();

    if (Actors.Num() == 0 || !Actors[0])
    {
        OutError = TEXT("No Actors selected");
        return false;
    }

    TArray<FMASceneGraphNode> MatchingNodes;
    if (!FindMatchingNodes(World, Actors[0]->GetActorGuid().ToString(), MatchingNodes, OutError))
    {
        return false;
    }

    TSet<FString> SelectedGuids;
    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            SelectedGuids.Add(Actor->GetActorGuid().ToString());
        }
    }

    for (const FMASceneGraphNode& Node : MatchingNodes)
    {
        if (Node.GuidArray.Num() == 0)
        {
            continue;
        }

        TSet<FString> NodeGuids;
        for (const FString& Guid : Node.GuidArray)
        {
            NodeGuids.Add(Guid);
        }

        bool bAllMatch = true;
        for (const FString& SelectedGuid : SelectedGuids)
        {
            if (!NodeGuids.Contains(SelectedGuid))
            {
                bAllMatch = false;
                break;
            }
        }

        if (bAllMatch)
        {
            OutNode = Node;
            return true;
        }
    }

    OutError = TEXT("Selected actors do not belong to the same existing node");
    return false;
}

FMAModifyPreviewModel FMAModifyWidgetSceneGraphAdapter::BuildSingleActorPreview(UWorld* World, AActor* Actor) const
{
    FMAModifyPreviewModel Preview;

    if (!Actor)
    {
        return Preview;
    }

    if (!World)
    {
        Preview.Text = TEXT("Unable to get World");
        Preview.Tone = EMAModifyPreviewTone::Error;
        return Preview;
    }

    const FString ActorGuid = Actor->GetActorGuid().ToString();
    TArray<FMASceneGraphNode> MatchingNodes;
    FString MatchError;
    if (FindMatchingNodes(World, ActorGuid, MatchingNodes, MatchError))
    {
        FString CombinedPreview;
        if (MatchingNodes.Num() > 1)
        {
            CombinedPreview = FString::Printf(TEXT("This Actor belongs to %d nodes:\n"), MatchingNodes.Num());
            CombinedPreview += TEXT("━━━━━━━━━━━━━━━━━━━━\n\n");
        }

        for (int32 Index = 0; Index < MatchingNodes.Num(); ++Index)
        {
            if (Index > 0)
            {
                CombinedPreview += TEXT("\n────────────────────\n\n");
            }
            CombinedPreview += FormatJsonPreviewWithHighlight(MatchingNodes[Index], ActorGuid);
        }

        Preview.Text = CombinedPreview;
        Preview.Tone = EMAModifyPreviewTone::Success;
        return Preview;
    }

    const FVector Location = Actor->GetActorLocation();
    Preview.Text = FString::Printf(
        TEXT("Actor: %s\nGUID: %s\nLocation: [%.0f, %.0f, %.0f]\n\n(Not annotated - does not belong to any node)"),
        *Actor->GetName(),
        *ActorGuid,
        Location.X,
        Location.Y,
        Location.Z);
    Preview.Tone = EMAModifyPreviewTone::Warning;
    return Preview;
}

FMAModifyPreviewModel FMAModifyWidgetSceneGraphAdapter::BuildMultiActorPreview(const TArray<AActor*>& Actors) const
{
    FMAModifyPreviewModel Preview;

    if (Actors.Num() == 0)
    {
        return Preview;
    }

    Preview.Text = FString::Printf(TEXT("Multi-select mode: %d Actors\n\n"), Actors.Num());
    Preview.Text += TEXT("Selected Actors:\n");
    for (int32 Index = 0; Index < FMath::Min(Actors.Num(), 5); ++Index)
    {
        if (Actors[Index])
        {
            const FString Guid = Actors[Index]->GetActorGuid().ToString();
            Preview.Text += FString::Printf(
                TEXT("  %d. %s\n     GUID: %s\n"),
                Index + 1,
                *Actors[Index]->GetName(),
                *Guid.Left(20));
        }
    }

    if (Actors.Num() > 5)
    {
        Preview.Text += FString::Printf(TEXT("  ... and %d more\n"), Actors.Num() - 5);
    }

    Preview.Text += TEXT("\nEnter shape:polygon or shape:linestring");
    Preview.Tone = EMAModifyPreviewTone::Info;
    return Preview;
}

FString FMAModifyWidgetSceneGraphAdapter::BuildEditNodeJson(UWorld* World, const FMAAnnotationInput& Input, const FString& NodeId) const
{
    UMASceneGraphManager* SceneGraphManager = ResolveSceneGraphManager(World);
    if (!SceneGraphManager)
    {
        return TEXT("{}");
    }

    const FMASceneGraphNode OriginalNode = SceneGraphManager->GetNodeById(NodeId);
    if (!OriginalNode.IsValid())
    {
        return TEXT("{}");
    }

    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(OriginalNode.RawJson);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        return TEXT("{}");
    }

    TSharedPtr<FJsonObject> PropertiesObject = RootObject->GetObjectField(TEXT("properties"));
    if (!PropertiesObject.IsValid())
    {
        PropertiesObject = MakeShared<FJsonObject>();
        RootObject->SetObjectField(TEXT("properties"), PropertiesObject);
    }

    PropertiesObject->SetStringField(TEXT("type"), Input.Type);
    if (Input.HasCategory())
    {
        PropertiesObject->SetStringField(TEXT("category"), Input.GetCategoryString());
    }

    for (const TPair<FString, FString>& Prop : Input.Properties)
    {
        if (Prop.Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) ||
            Prop.Value.Equals(TEXT("false"), ESearchCase::IgnoreCase))
        {
            PropertiesObject->SetBoolField(Prop.Key, Prop.Value.Equals(TEXT("true"), ESearchCase::IgnoreCase));
        }
        else
        {
            PropertiesObject->SetStringField(Prop.Key, Prop.Value);
        }
    }

    RootObject->SetBoolField(TEXT("_edit_mode"), true);
    RootObject->SetStringField(TEXT("_edit_node_id"), NodeId);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    return OutputString;
}
