#include "MAModifyWidgetCoordinator.h"

bool FMAModifyWidgetCoordinator::PrepareSubmission(
    UWorld* World,
    const TArray<AActor*>& SelectedActors,
    EMAAnnotationMode AnnotationMode,
    const FString& EditingNodeId,
    const FString& LabelContent,
    FMAModifyWidgetSubmission& OutSubmission) const
{
    OutSubmission = FMAModifyWidgetSubmission();

    if (!World || SelectedActors.Num() == 0 || !SelectedActors[0])
    {
        return false;
    }

    FMAAnnotationInput ParsedInput;
    FString ParseError;
    if (!InputParser.ParseAnnotationInput(World, LabelContent, ParsedInput, ParseError))
    {
        OutSubmission.Kind = EMAModifySubmissionKind::Single;
        OutSubmission.PrimaryActor = SelectedActors[0];
        OutSubmission.LabelText = LabelContent;
        return true;
    }

    if (AnnotationMode == EMAAnnotationMode::AddNew && ParsedInput.HasCategory())
    {
        FString ValidationError;
        if (!InputParser.ValidateSelectionForCategory(ParsedInput.Category, SelectedActors.Num(), ValidationError))
        {
            OutSubmission.Kind = EMAModifySubmissionKind::Single;
            OutSubmission.PrimaryActor = SelectedActors[0];
            OutSubmission.LabelText = FString::Printf(TEXT("ERROR: %s"), *ValidationError);
            return true;
        }
    }

    if (AnnotationMode == EMAAnnotationMode::EditExisting)
    {
        OutSubmission.Kind = EMAModifySubmissionKind::Multi;
        OutSubmission.Actors = SelectedActors;
        OutSubmission.LabelText = LabelContent;
        OutSubmission.GeneratedJson = SceneGraphAdapter.BuildEditNodeJson(World, ParsedInput, EditingNodeId);
        return true;
    }

    if (ParsedInput.HasCategory())
    {
        OutSubmission.Kind = EMAModifySubmissionKind::Multi;
        OutSubmission.Actors = SelectedActors;
        OutSubmission.LabelText = LabelContent;
        OutSubmission.GeneratedJson = NodeBuilder.GenerateSceneGraphNodeV2(World, ParsedInput, SelectedActors);
        return true;
    }

    if (ParsedInput.IsMultiSelect())
    {
        OutSubmission.Kind = EMAModifySubmissionKind::Multi;
        OutSubmission.Actors = SelectedActors;
        OutSubmission.LabelText = LabelContent;
        OutSubmission.GeneratedJson = NodeBuilder.GenerateSceneGraphNode(World, ParsedInput, SelectedActors);
        return true;
    }

    OutSubmission.Kind = EMAModifySubmissionKind::Single;
    OutSubmission.PrimaryActor = SelectedActors[0];
    OutSubmission.LabelText = LabelContent;
    return true;
}
