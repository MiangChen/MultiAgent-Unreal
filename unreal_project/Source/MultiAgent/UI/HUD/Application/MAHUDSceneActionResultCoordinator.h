#pragma once

#include "CoreMinimal.h"
#include "../../Mode/Application/MAModifyWidgetStateCoordinator.h"
#include "../../Mode/Domain/MASceneActionResult.h"

class AActor;
class AMAHUD;
class UMAModifyWidget;

class MULTIAGENT_API FMAHUDSceneActionResultCoordinator
{
public:
    void ApplyResult(AMAHUD* HUD, const FMASceneActionResult& Result) const;
    void ApplyModifySelection(UMAModifyWidget* ModifyWidget, AActor* SelectedActor) const;
    void ApplyModifySelection(UMAModifyWidget* ModifyWidget, const TArray<AActor*>& SelectedActors) const;
    void ApplyModifyResult(
        AMAHUD* HUD,
        UMAModifyWidget* ModifyWidget,
        const TArray<AActor*>& Actors,
        const FString& LabelText,
        const FMASceneActionResult& Result) const;

private:
    FMAModifyWidgetStateCoordinator ModifyWidgetStateCoordinator;
};
