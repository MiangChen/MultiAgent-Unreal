#include "MAModifyWidgetRuntimeAdapter.h"

#include "Blueprint/UserWidget.h"
#include "MAModifyWidgetInputParser.h"
#include "MAModifyWidgetNodeBuilder.h"
#include "MAModifyWidgetSceneGraphAdapter.h"

namespace
{
UWorld* ResolveWorld(const UUserWidget* Context)
{
    return Context ? Context->GetWorld() : nullptr;
}
}

FMAModifyWidgetSelectionModels FMAModifyWidgetRuntimeAdapter::BuildSingleSelectionModels(
    const UUserWidget* Context,
    AActor* Actor,
    const FString& DefaultHintText,
    const FString& MultiSelectHintText) const
{
    FMAModifyWidgetSelectionModels Models;
    FMAModifyWidgetSceneGraphAdapter SceneGraphAdapter;
    UWorld* World = ResolveWorld(Context);

    if (!Actor)
    {
        Models.SelectionViewModel = FMAModifyWidgetModel::BuildSelectionViewModel(0, TEXT(""), nullptr, DefaultHintText, MultiSelectHintText);
        return Models;
    }

    TArray<FMASceneGraphNode> MatchingNodes;
    FString MatchError;
    const bool bHasMatchingNode = SceneGraphAdapter.FindMatchingNodes(
        World,
        Actor->GetActorGuid().ToString(),
        MatchingNodes,
        MatchError) && MatchingNodes.Num() > 0;
    const FMASceneGraphNode* MatchedNode = bHasMatchingNode ? &MatchingNodes[0] : nullptr;

    Models.SelectionViewModel = FMAModifyWidgetModel::BuildSelectionViewModel(
        1,
        Actor->GetName(),
        MatchedNode,
        DefaultHintText,
        MultiSelectHintText);
    Models.PreviewModel = SceneGraphAdapter.BuildSingleActorPreview(World, Actor);
    return Models;
}

FMAModifyWidgetSelectionModels FMAModifyWidgetRuntimeAdapter::BuildMultiSelectionModels(
    const UUserWidget* Context,
    const TArray<AActor*>& Actors,
    const FString& DefaultHintText,
    const FString& MultiSelectHintText) const
{
    FMAModifyWidgetSelectionModels Models;
    FMAModifyWidgetSceneGraphAdapter SceneGraphAdapter;
    UWorld* World = ResolveWorld(Context);

    if (Actors.Num() == 0)
    {
        Models.SelectionViewModel = FMAModifyWidgetModel::BuildSelectionViewModel(0, TEXT(""), nullptr, DefaultHintText, MultiSelectHintText);
        return Models;
    }

    FMASceneGraphNode MatchedNode;
    FString MatchError;
    const bool bFoundMatchingNode = SceneGraphAdapter.FindSharedNodeForActors(World, Actors, MatchedNode, MatchError);
    Models.SelectionViewModel = FMAModifyWidgetModel::BuildSelectionViewModel(
        Actors.Num(),
        Actors.Num() == 1 && Actors[0] ? Actors[0]->GetName() : TEXT(""),
        bFoundMatchingNode ? &MatchedNode : nullptr,
        DefaultHintText,
        MultiSelectHintText);
    Models.PreviewModel = Actors.Num() > 1
        ? SceneGraphAdapter.BuildMultiActorPreview(Actors)
        : SceneGraphAdapter.BuildSingleActorPreview(World, Actors[0]);
    return Models;
}

FMAModifyWidgetSubmitResult FMAModifyWidgetRuntimeAdapter::BuildSubmitResult(
    const UUserWidget* Context,
    const TArray<AActor*>& SelectedActors,
    const FString& LabelText,
    EMAAnnotationMode AnnotationMode,
    const FString& EditingNodeId) const
{
    FMAModifyWidgetSubmitResult Result;
    UWorld* World = ResolveWorld(Context);
    if (SelectedActors.Num() == 0)
    {
        return Result;
    }

    FMAModifyWidgetInputParser InputParser;
    FMAModifyWidgetNodeBuilder NodeBuilder;
    FMAModifyWidgetSceneGraphAdapter SceneGraphAdapter;

    FMAAnnotationInput ParsedInput;
    FString ParseError;
    if (!InputParser.ParseAnnotationInput(World, LabelText, ParsedInput, ParseError))
    {
        Result.Kind = EMAModifyWidgetSubmitKind::Single;
        Result.LabelText = LabelText;
        return Result;
    }

    if (AnnotationMode == EMAAnnotationMode::AddNew && ParsedInput.HasCategory())
    {
        FString ValidationError;
        if (!InputParser.ValidateSelectionForCategory(ParsedInput.Category, SelectedActors.Num(), ValidationError))
        {
            Result.Kind = EMAModifyWidgetSubmitKind::Single;
            Result.LabelText = FString::Printf(TEXT("ERROR: %s"), *ValidationError);
            return Result;
        }
    }

    if (AnnotationMode == EMAAnnotationMode::EditExisting)
    {
        Result.Kind = EMAModifyWidgetSubmitKind::Multi;
        Result.LabelText = LabelText;
        Result.GeneratedJson = SceneGraphAdapter.BuildEditNodeJson(World, ParsedInput, EditingNodeId);
        return Result;
    }

    if (ParsedInput.HasCategory())
    {
        Result.Kind = EMAModifyWidgetSubmitKind::Multi;
        Result.LabelText = LabelText;
        Result.GeneratedJson = NodeBuilder.GenerateSceneGraphNodeV2(World, ParsedInput, SelectedActors);
        return Result;
    }

    if (ParsedInput.IsMultiSelect())
    {
        Result.Kind = EMAModifyWidgetSubmitKind::Multi;
        Result.LabelText = LabelText;
        Result.GeneratedJson = NodeBuilder.GenerateSceneGraphNode(World, ParsedInput, SelectedActors);
        return Result;
    }

    Result.Kind = EMAModifyWidgetSubmitKind::Single;
    Result.LabelText = LabelText;
    return Result;
}
