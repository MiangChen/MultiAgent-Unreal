#pragma once

#include "CoreMinimal.h"
#include "MAEditWidgetActionRequest.h"
#include "MAEditWidgetSelectionState.h"
#include "MAEditWidgetStateCoordinator.h"
#include "../Domain/MAEditWidgetModel.h"

class AMAHUD;
class UMAEditWidget;
class AMAPointOfInterest;

class MULTIAGENT_API FMAEditWidgetCoordinator
{
public:
    void RefreshFromEditModeSelection(AMAHUD* HUD, UMAEditWidget* Widget);
    void RefreshCurrentSelection(AMAHUD* HUD, UMAEditWidget* Widget);

    bool HandleConfirmRequested(UMAEditWidget* Widget, const FString& JsonContent, FMAEditWidgetActionRequest& OutRequest);
    bool HandleDeleteActorRequested(UMAEditWidget* Widget, FMAEditWidgetActionRequest& OutRequest);
    bool HandleCreateGoalRequested(UMAEditWidget* Widget, const FString& Description, FMAEditWidgetActionRequest& OutRequest);
    bool HandleCreateZoneRequested(UMAEditWidget* Widget, const FString& Description, FMAEditWidgetActionRequest& OutRequest);
    bool HandleAddPresetActorRequested(UMAEditWidget* Widget, const FString& ActorType, FMAEditWidgetActionRequest& OutRequest);
    bool HandleDeletePOIsRequested(UMAEditWidget* Widget, FMAEditWidgetActionRequest& OutRequest);
    bool HandleSetAsGoalRequested(UMAEditWidget* Widget, FMAEditWidgetActionRequest& OutRequest);
    bool HandleUnsetAsGoalRequested(UMAEditWidget* Widget, FMAEditWidgetActionRequest& OutRequest);
    void HandleNodeSwitchRequested(AMAHUD* HUD, UMAEditWidget* Widget, int32 NodeIndex);

private:
    void ApplyCurrentViewModel(UMAEditWidget* Widget, const FString& ErrorMessage = TEXT(""));
    void ClearSelection();
    void SetActorSelection(AMAHUD* HUD, AActor* Actor);
    void SetPOISelection(const TArray<AMAPointOfInterest*>& POIs);

    FMAEditWidgetViewModel BuildViewModel(const UMAEditWidget* Widget, const FString& ErrorMessage = TEXT("")) const;

private:
    FMAEditWidgetSelectionState SelectionState;
    FMAEditWidgetStateCoordinator WidgetStateCoordinator;
};
