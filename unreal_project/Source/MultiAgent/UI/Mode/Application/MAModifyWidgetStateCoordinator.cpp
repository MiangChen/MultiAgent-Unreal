// Modify widget state application helper.

#include "MAModifyWidgetStateCoordinator.h"

#include "../MAModifyWidget.h"

void FMAModifyWidgetStateCoordinator::ApplySingleSelection(UMAModifyWidget* Widget, AActor* SelectedActor) const
{
    if (!Widget)
    {
        return;
    }

    Widget->SetSelectedActor(SelectedActor);
}

void FMAModifyWidgetStateCoordinator::ApplyMultiSelection(UMAModifyWidget* Widget, const TArray<AActor*>& SelectedActors) const
{
    if (!Widget)
    {
        return;
    }

    Widget->SetSelectedActors(SelectedActors);
}

void FMAModifyWidgetStateCoordinator::ResetWidget(UMAModifyWidget* Widget) const
{
    if (!Widget)
    {
        return;
    }

    Widget->ClearSelection();
}

void FMAModifyWidgetStateCoordinator::ApplyActionResult(
    UMAModifyWidget* Widget,
    const TArray<AActor*>& Actors,
    const FString& LabelText,
    const FMASceneActionResult& Result) const
{
    if (!Widget)
    {
        return;
    }

    if (!Result.bSuccess)
    {
        if (Result.bClearSelection)
        {
            Widget->ClearSelection();
        }
        else if (Actors.Num() > 1)
        {
            Widget->SetSelectedActors(Actors);
        }
        else if (Actors.Num() == 1)
        {
            Widget->SetSelectedActor(Actors[0]);
        }

        Widget->SetLabelText(LabelText);
        return;
    }

    if (Result.bClearSelection)
    {
        Widget->ClearSelection();
    }
}
