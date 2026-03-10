#include "MAModifyWidgetCoordinator.h"

#include "../MAModifyWidget.h"

FMAModifySelectionViewModel FMAModifyWidgetCoordinator::BuildClearedSelectionViewModel(
    const FString& DefaultHintText,
    const FString& MultiSelectHintText) const
{
    return FMAModifyWidgetModel::BuildSelectionViewModel(0, TEXT(""), nullptr, DefaultHintText, MultiSelectHintText);
}

FMAModifyPreviewModel FMAModifyWidgetCoordinator::BuildClearedPreviewModel() const
{
    return FMAModifyPreviewModel();
}

FMAModifyWidgetSelectionModels FMAModifyWidgetCoordinator::BuildSingleSelectionModels(UMAModifyWidget* Widget, AActor* Actor) const
{
    if (!Widget)
    {
        return FMAModifyWidgetSelectionModels();
    }

    return Widget->RuntimeBuildSingleSelectionModels(Actor);
}

FMAModifyWidgetSelectionModels FMAModifyWidgetCoordinator::BuildMultiSelectionModels(
    UMAModifyWidget* Widget,
    const TArray<AActor*>& Actors) const
{
    if (!Widget)
    {
        return FMAModifyWidgetSelectionModels();
    }

    return Widget->RuntimeBuildMultiSelectionModels(Actors);
}

FMAModifyWidgetSubmitResult FMAModifyWidgetCoordinator::BuildSubmitResult(
    UMAModifyWidget* Widget,
    const TArray<AActor*>& SelectedActors,
    const FString& LabelText,
    EMAAnnotationMode AnnotationMode,
    const FString& EditingNodeId) const
{
    if (!Widget)
    {
        return FMAModifyWidgetSubmitResult();
    }

    return Widget->RuntimeBuildSubmitResult(SelectedActors, LabelText, AnnotationMode, EditingNodeId);
}
