#include "MAModifySceneActionAdapter.h"

#include "../../../Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "../../../Core/SceneGraph/Infrastructure/UE/MAUESceneApplier.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    UMASceneGraphManager* ResolveModifyActionSceneGraphManager(UWorld* World)
    {
        UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
        return GameInstance ? GameInstance->GetSubsystem<UMASceneGraphManager>() : nullptr;
    }
}

FMASceneActionResult FMAModifySceneActionAdapter::ApplySingleModify(
    UWorld* World,
    AActor* Actor,
    const FString& LabelText) const
{
    FMASceneActionResult Result;

    if (!World)
    {
        Result.Message = TEXT("Save failed: World not found");
        return Result;
    }
    if (!Actor)
    {
        Result.Message = TEXT("Please select an Actor first");
        return Result;
    }

    UMASceneGraphManager* SceneGraphManager = ResolveModifyActionSceneGraphManager(World);
    if (!SceneGraphManager)
    {
        Result.Message = TEXT("Save failed: SceneGraphManager not found");
        return Result;
    }

    FString Id;
    FString Type;
    FString ErrorMessage;
    if (!SceneGraphManager->ParseLabelInput(LabelText, Id, Type, ErrorMessage))
    {
        Result.Message = ErrorMessage;
        return Result;
    }

    FString GeneratedLabel;
    if (!SceneGraphManager->AddSceneNode(Id, Type, Actor->GetActorLocation(), Actor, GeneratedLabel, ErrorMessage))
    {
        Result.Message = ErrorMessage;
        Result.bIsWarning = ErrorMessage.Contains(TEXT("ID already exists"));
        Result.bClearSelection = Result.bIsWarning;
        Result.bClearHighlightedActor = Result.bIsWarning;
        return Result;
    }

    Result.bSuccess = true;
    Result.bClearSelection = true;
    Result.bReloadSceneVisualization = true;
    Result.bClearHighlightedActor = true;
    Result.Message = FString::Printf(TEXT("Label saved: %s"), *GeneratedLabel);
    return Result;
}

FMASceneActionResult FMAModifySceneActionAdapter::ApplyMultiModify(
    UWorld* World,
    const TArray<AActor*>& Actors,
    const FString& LabelText,
    const FString& GeneratedJson) const
{
    FMASceneActionResult Result;

    if (!World)
    {
        Result.Message = TEXT("Save failed: World not found");
        return Result;
    }
    if (Actors.Num() == 0)
    {
        Result.Message = TEXT("Please select at least one Actor first");
        return Result;
    }

    UMASceneGraphManager* SceneGraphManager = ResolveModifyActionSceneGraphManager(World);
    if (!SceneGraphManager)
    {
        Result.Message = TEXT("Save failed: SceneGraphManager not found");
        return Result;
    }

    FString ErrorMessage;
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(GeneratedJson);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        Result.Message = TEXT("JSON parse failed");
        return Result;
    }

    bool bIsEditMode = false;
    FString EditNodeId;
    if (JsonObject->TryGetBoolField(TEXT("_edit_mode"), bIsEditMode) && bIsEditMode)
    {
        JsonObject->TryGetStringField(TEXT("_edit_node_id"), EditNodeId);
        JsonObject->RemoveField(TEXT("_edit_mode"));
        JsonObject->RemoveField(TEXT("_edit_node_id"));

        FString CleanJson;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&CleanJson);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

        if (!SceneGraphManager->EditNode(EditNodeId, CleanJson, ErrorMessage))
        {
            Result.Message = ErrorMessage;
            return Result;
        }

        const FMASceneGraphNode UpdatedNode = SceneGraphManager->GetNodeById(EditNodeId);
        if (UpdatedNode.IsValid())
        {
            FMAUESceneApplier::ApplyNodeToScene(World, UpdatedNode);
        }

        SceneGraphManager->SaveToSource();
        Result.bSuccess = true;
        Result.bClearSelection = true;
        Result.bReloadSceneVisualization = true;
        Result.bClearHighlightedActor = true;
        Result.Message = FString::Printf(TEXT("Node %s updated"), *EditNodeId);
        return Result;
    }

    FString Id;
    FString Type;
    if (!SceneGraphManager->ParseLabelInput(LabelText, Id, Type, ErrorMessage))
    {
        Result.Message = ErrorMessage;
        return Result;
    }

    if (SceneGraphManager->IsIdExists(Id))
    {
        Result.Message = FString::Printf(TEXT("ID '%s' already exists, please use a different ID"), *Id);
        Result.bIsWarning = true;
        return Result;
    }

    if (!SceneGraphManager->AddNode(GeneratedJson, ErrorMessage))
    {
        Result.Message = ErrorMessage;
        return Result;
    }

    SceneGraphManager->SaveToSource();
    Result.bSuccess = true;
    Result.bClearSelection = true;
    Result.bReloadSceneVisualization = true;
    Result.bClearHighlightedActor = true;
    Result.Message = FString::Printf(TEXT("Multi-select annotation saved: %d Actors"), Actors.Num());
    return Result;
}
