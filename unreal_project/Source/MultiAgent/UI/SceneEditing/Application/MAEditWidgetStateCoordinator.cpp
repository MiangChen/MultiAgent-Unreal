// Edit widget state application helper.

#include "MAEditWidgetStateCoordinator.h"

#include "../MAEditWidget.h"

void FMAEditWidgetStateCoordinator::ApplyViewModel(UMAEditWidget* Widget, const FMAEditWidgetViewModel& ViewModel) const
{
    if (!Widget)
    {
        return;
    }

    Widget->ApplyViewModel(ViewModel);
}

void FMAEditWidgetStateCoordinator::ResetWidget(UMAEditWidget* Widget) const
{
    if (!Widget)
    {
        return;
    }

    ApplyViewModel(Widget, FMAEditWidgetViewModel());
}
