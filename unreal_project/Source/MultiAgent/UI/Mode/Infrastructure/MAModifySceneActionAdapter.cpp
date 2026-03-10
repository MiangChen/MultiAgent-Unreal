#include "MAModifySceneActionAdapter.h"

#include "../../../Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "../../../Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.h"
#include "../../../Core/SceneGraph/Feedback/MASceneGraphFeedback.h"
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
        return FMASceneGraphBootstrap::Resolve(World);
    }

    FMASceneActionResult ToModifySceneActionResult(const FMASceneGraphMutationFeedback& Feedback)
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
}

FMASceneActionResult FMAModifySceneActionAdapter::ApplySingleModify(
    UWorld* World,
    AActor* Actor,
    const FString& LabelText) const
{
    if (!World)
    {
        return ToModifySceneActionResult(MASceneGraphFeedback::MakeMutationFailure(TEXT("Save failed: World not found")));
    }
    if (!Actor)
    {
        return ToModifySceneActionResult(MASceneGraphFeedback::MakeMutationFailure(TEXT("Please select an Actor first")));
    }

    UMASceneGraphManager* SceneGraphManager = ResolveModifyActionSceneGraphManager(World);
    if (!SceneGraphManager)
    {
        return ToModifySceneActionResult(MASceneGraphFeedback::MakeMutationFailure(TEXT("Save failed: SceneGraphManager not found")));
    }

    FString Id;
    FString Type;
    FString ErrorMessage;
    if (!SceneGraphManager->ParseLabelInput(LabelText, Id, Type, ErrorMessage))
    {
        return ToModifySceneActionResult(MASceneGraphFeedback::MakeMutationFailure(ErrorMessage));
    }

    FString GeneratedLabel;
    if (!SceneGraphManager->AddSceneNode(Id, Type, Actor->GetActorLocation(), Actor, GeneratedLabel, ErrorMessage))
    {
        return ToModifySceneActionResult(
            ErrorMessage.Contains(TEXT("ID already exists"))
                ? MASceneGraphFeedback::MakeMutationWarning(ErrorMessage, true, true)
                : MASceneGraphFeedback::MakeMutationFailure(ErrorMessage));
    }

    return ToModifySceneActionResult(
        MASceneGraphFeedback::MakeMutationSuccess(
            FString::Printf(TEXT("Label saved: %s"), *GeneratedLabel),
            true,
            false,
            false,
            true,
            true));
}

FMASceneActionResult FMAModifySceneActionAdapter::ApplyMultiModify(
    UWorld* World,
    const TArray<AActor*>& Actors,
    const FString& LabelText,
    const FString& GeneratedJson) const
{
    if (!World)
    {
        return ToModifySceneActionResult(MASceneGraphFeedback::MakeMutationFailure(TEXT("Save failed: World not found")));
    }
    if (Actors.Num() == 0)
    {
        return ToModifySceneActionResult(MASceneGraphFeedback::MakeMutationFailure(TEXT("Please select at least one Actor first")));
    }

    UMASceneGraphManager* SceneGraphManager = ResolveModifyActionSceneGraphManager(World);
    if (!SceneGraphManager)
    {
        return ToModifySceneActionResult(MASceneGraphFeedback::MakeMutationFailure(TEXT("Save failed: SceneGraphManager not found")));
    }

    FString ErrorMessage;
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(GeneratedJson);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return ToModifySceneActionResult(MASceneGraphFeedback::MakeMutationFailure(TEXT("JSON parse failed")));
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
            return ToModifySceneActionResult(MASceneGraphFeedback::MakeMutationFailure(ErrorMessage));
        }

        const FMASceneGraphNode UpdatedNode = SceneGraphManager->GetNodeById(EditNodeId);
        if (UpdatedNode.IsValid())
        {
            FMAUESceneApplier::ApplyNodeToScene(World, UpdatedNode);
        }

        SceneGraphManager->SaveToSource();
        return ToModifySceneActionResult(
            MASceneGraphFeedback::MakeMutationSuccess(
                FString::Printf(TEXT("Node %s updated"), *EditNodeId),
                true,
                false,
                false,
                true,
                true));
    }

    FString Id;
    FString Type;
    if (!SceneGraphManager->ParseLabelInput(LabelText, Id, Type, ErrorMessage))
    {
        return ToModifySceneActionResult(MASceneGraphFeedback::MakeMutationFailure(ErrorMessage));
    }

    if (SceneGraphManager->IsIdExists(Id))
    {
        return ToModifySceneActionResult(
            MASceneGraphFeedback::MakeMutationWarning(
                FString::Printf(TEXT("ID '%s' already exists, please use a different ID"), *Id)));
    }

    if (!SceneGraphManager->AddNode(GeneratedJson, ErrorMessage))
    {
        return ToModifySceneActionResult(MASceneGraphFeedback::MakeMutationFailure(ErrorMessage));
    }

    SceneGraphManager->SaveToSource();
    return ToModifySceneActionResult(
        MASceneGraphFeedback::MakeMutationSuccess(
            FString::Printf(TEXT("Multi-select annotation saved: %d Actors"), Actors.Num()),
            true,
            false,
            false,
            true,
            true));
}
