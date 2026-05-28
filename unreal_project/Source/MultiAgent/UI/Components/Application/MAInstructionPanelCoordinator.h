#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAInstructionPanelModels.h"

class UUserWidget;

class FMAInstructionPanelCoordinator
{
public:
    FMAInstructionPanelActionResult BuildSubmitResult(const FString& Command) const;
    void ApplyPostSubmit(const UUserWidget* Context, const FMAInstructionPanelActionResult& Result) const;
};
