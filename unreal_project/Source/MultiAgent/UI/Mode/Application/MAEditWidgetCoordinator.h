#pragma once

#include "CoreMinimal.h"
#include "MAEditWidgetStateCoordinator.h"
#include "../Domain/MAEditWidgetModel.h"
#include "../Infrastructure/MAEditWidgetSceneGraphAdapter.h"

class AMAHUD;
class UMAEditWidget;
class AMAPointOfInterest;

class MULTIAGENT_API FMAEditWidgetCoordinator
{
public:
    void RefreshFromEditModeSelection(AMAHUD* HUD, UMAEditWidget* Widget);
    void RefreshCurrentSelection(AMAHUD* HUD, UMAEditWidget* Widget);

    void HandleConfirmRequested(AMAHUD* HUD, UMAEditWidget* Widget, const FString& JsonContent);
    void HandleDeleteActorRequested(AMAHUD* HUD, UMAEditWidget* Widget);
    void HandleCreateGoalRequested(AMAHUD* HUD, UMAEditWidget* Widget, const FString& Description);
    void HandleCreateZoneRequested(AMAHUD* HUD, UMAEditWidget* Widget, const FString& Description);
    void HandleAddPresetActorRequested(AMAHUD* HUD, UMAEditWidget* Widget, const FString& ActorType);
    void HandleDeletePOIsRequested(AMAHUD* HUD, UMAEditWidget* Widget);
    void HandleSetAsGoalRequested(AMAHUD* HUD, UMAEditWidget* Widget);
    void HandleUnsetAsGoalRequested(AMAHUD* HUD, UMAEditWidget* Widget);
    void HandleNodeSwitchRequested(AMAHUD* HUD, UMAEditWidget* Widget, int32 NodeIndex);

private:
    void ApplyCurrentViewModel(UMAEditWidget* Widget, const FString& ErrorMessage = TEXT(""));
    void ClearSelection();
    void SetActorSelection(AMAHUD* HUD, AActor* Actor);
    void SetPOISelection(const TArray<AMAPointOfInterest*>& POIs);

    FMAEditWidgetViewModel BuildViewModel(const FString& ErrorMessage = TEXT("")) const;

private:
    FMAEditWidgetSelectionState SelectionState;
    FMAEditWidgetStateCoordinator WidgetStateCoordinator;
    FMAEditWidgetSceneGraphAdapter SceneGraphAdapter;
};
