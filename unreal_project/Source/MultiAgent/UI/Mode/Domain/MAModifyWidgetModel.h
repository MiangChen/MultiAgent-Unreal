#pragma once

#include "CoreMinimal.h"
#include "MASceneSelectionDisplay.h"
#include "../MAModifyTypes.h"
#include "../../../Core/Manager/MASceneGraphNodeTypes.h"

class AActor;

enum class EMAModifyPreviewTone : uint8
{
    Default,
    Info,
    Success,
    Warning,
    Error
};

struct FMAModifyPreviewModel
{
    FString Text = TEXT("Select an Actor to display JSON preview");
    EMAModifyPreviewTone Tone = EMAModifyPreviewTone::Default;
};

struct FMAModifySelectionViewModel
{
    EMAAnnotationMode AnnotationMode = EMAAnnotationMode::AddNew;
    FString EditingNodeId;
    FString HintText;
    FString LabelInputText;
    FString LabelInputHintText;
};

struct FMAModifyWidgetModel
{
    static FString BuildInputTextFromNode(const FMASceneGraphNode& Node)
    {
        TArray<FString> Parts;

        if (!Node.Category.IsEmpty())
        {
            Parts.Add(FString::Printf(TEXT("cate:%s"), *Node.Category));
        }

        if (!Node.Type.IsEmpty())
        {
            Parts.Add(FString::Printf(TEXT("type:%s"), *Node.Type));
        }

        for (const TPair<FString, FString>& Feature : Node.Features)
        {
            if (Feature.Key != TEXT("label"))
            {
                Parts.Add(FString::Printf(TEXT("%s:%s"), *Feature.Key, *Feature.Value));
            }
        }

        return FString::Join(Parts, TEXT(","));
    }

    static FMAModifySelectionViewModel BuildSelectionViewModel(
        const TArray<AActor*>& Actors,
        const FMASceneGraphNode* MatchedNode,
        const FString& DefaultHintText,
        const FString& MultiSelectHintText)
    {
        FMAModifySelectionViewModel ViewModel;

        if (MatchedNode)
        {
            ViewModel.AnnotationMode = EMAAnnotationMode::EditExisting;
            ViewModel.EditingNodeId = MatchedNode->Id;
            ViewModel.LabelInputText = BuildInputTextFromNode(*MatchedNode);
        }
        else
        {
            ViewModel.AnnotationMode = EMAAnnotationMode::AddNew;
            ViewModel.LabelInputHintText = Actors.Num() > 1 ? MultiSelectHintText : DefaultHintText;
        }

        ViewModel.HintText = FMASceneSelectionDisplay::BuildActorSelectionHint(Actors, DefaultHintText);
        if (Actors.Num() == 0)
        {
            ViewModel.LabelInputHintText = DefaultHintText;
        }

        return ViewModel;
    }
};
