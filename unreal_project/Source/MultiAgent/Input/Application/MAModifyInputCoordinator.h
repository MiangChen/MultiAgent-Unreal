#pragma once

#include "CoreMinimal.h"
#include "../Infrastructure/MAActorHighlightAdapter.h"
#include "../Infrastructure/MAHUDInputAdapter.h"

class AMAPlayerController;
class AActor;

class MULTIAGENT_API FMAModifyInputCoordinator
{
public:
    void HandleLeftClick(AMAPlayerController* PlayerController) const;
    void EnterMode(AMAPlayerController* PlayerController) const;
    void ExitMode(AMAPlayerController* PlayerController) const;
    void ClearHighlight(AMAPlayerController* PlayerController) const;

private:
    void BroadcastSelection(AMAPlayerController* PlayerController) const;
    void ClearAllHighlights(AMAPlayerController* PlayerController) const;
    void AddToSelection(AMAPlayerController* PlayerController, AActor* Actor) const;
    void RemoveFromSelection(AMAPlayerController* PlayerController, AActor* Actor) const;
    void ClearAndSelect(AMAPlayerController* PlayerController, AActor* Actor) const;

    FMAActorHighlightAdapter HighlightAdapter;
    FMAHUDInputAdapter HUDInputAdapter;
};
