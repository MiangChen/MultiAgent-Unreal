#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAEditWidgetModel.h"

class UMAEditWidget;

class MULTIAGENT_API FMAEditWidgetStateCoordinator
{
public:
    void ApplyViewModel(UMAEditWidget* Widget, const FMAEditWidgetViewModel& ViewModel) const;
    void ResetWidget(UMAEditWidget* Widget) const;
};
