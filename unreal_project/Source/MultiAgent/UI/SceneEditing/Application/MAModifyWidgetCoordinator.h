#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAModifyWidgetModel.h"

class AActor;
class UMAModifyWidget;

class MULTIAGENT_API FMAModifyWidgetCoordinator
{
public:
    FMAModifySelectionViewModel BuildClearedSelectionViewModel(const FString& DefaultHintText, const FString& MultiSelectHintText) const;
    FMAModifyPreviewModel BuildClearedPreviewModel() const;

    FMAModifyWidgetSelectionModels BuildSingleSelectionModels(UMAModifyWidget* Widget, AActor* Actor) const;
    FMAModifyWidgetSelectionModels BuildMultiSelectionModels(UMAModifyWidget* Widget, const TArray<AActor*>& Actors) const;
    FMAModifyWidgetSubmitResult BuildSubmitResult(
        UMAModifyWidget* Widget,
        const TArray<AActor*>& SelectedActors,
        const FString& LabelText,
        EMAAnnotationMode AnnotationMode,
        const FString& EditingNodeId) const;
};
