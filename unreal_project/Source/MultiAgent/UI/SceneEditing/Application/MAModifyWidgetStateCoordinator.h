#pragma once

#include "CoreMinimal.h"
#include "../Domain/MASceneActionResult.h"

class AActor;
class UMAModifyWidget;

class MULTIAGENT_API FMAModifyWidgetStateCoordinator
{
public:
    void ApplySingleSelection(UMAModifyWidget* Widget, AActor* SelectedActor) const;
    void ApplyMultiSelection(UMAModifyWidget* Widget, const TArray<AActor*>& SelectedActors) const;
    void ResetWidget(UMAModifyWidget* Widget) const;
    void ApplyActionResult(
        UMAModifyWidget* Widget,
        const TArray<AActor*>& Actors,
        const FString& LabelText,
        const FMASceneActionResult& Result) const;
};
