#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAModifyWidgetModel.h"

class AActor;
class UUserWidget;

class MULTIAGENT_API FMAModifyWidgetRuntimeAdapter
{
public:
    FMAModifyWidgetSelectionModels BuildSingleSelectionModels(
        const UUserWidget* Context,
        AActor* Actor,
        const FString& DefaultHintText,
        const FString& MultiSelectHintText) const;

    FMAModifyWidgetSelectionModels BuildMultiSelectionModels(
        const UUserWidget* Context,
        const TArray<AActor*>& Actors,
        const FString& DefaultHintText,
        const FString& MultiSelectHintText) const;

    FMAModifyWidgetSubmitResult BuildSubmitResult(
        const UUserWidget* Context,
        const TArray<AActor*>& SelectedActors,
        const FString& LabelText,
        EMAAnnotationMode AnnotationMode,
        const FString& EditingNodeId) const;
};
