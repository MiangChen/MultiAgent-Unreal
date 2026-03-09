#include "MAHUDSceneActionResultCoordinator.h"

#include "../MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../Mode/MAModifyWidget.h"
#include "../../Mode/MASceneListWidget.h"

void FMAHUDSceneActionResultCoordinator::ApplyResult(AMAHUD* HUD, const FMASceneActionResult& Result) const
{
    if (!HUD)
    {
        return;
    }

    HUD->ShowNotification(Result.Message, !Result.bSuccess && !Result.bIsWarning, Result.bIsWarning);

    if (Result.bRefreshSceneList)
    {
        if (UMASceneListWidget* SceneListWidget = HUD->UIManager ? HUD->UIManager->GetSceneListWidget() : nullptr)
        {
            SceneListWidget->RefreshLists();
        }
    }

    if (Result.bReloadSceneVisualization)
    {
        HUD->LoadSceneGraphForVisualization();
    }

    if (Result.bRefreshSelection)
    {
        HUD->OnTempSceneGraphChanged();
    }
}

void FMAHUDSceneActionResultCoordinator::ApplyModifySelection(UMAModifyWidget* ModifyWidget, AActor* SelectedActor) const
{
    ModifyWidgetStateCoordinator.ApplySingleSelection(ModifyWidget, SelectedActor);
}

void FMAHUDSceneActionResultCoordinator::ApplyModifySelection(
    UMAModifyWidget* ModifyWidget,
    const TArray<AActor*>& SelectedActors) const
{
    ModifyWidgetStateCoordinator.ApplyMultiSelection(ModifyWidget, SelectedActors);
}

void FMAHUDSceneActionResultCoordinator::ApplyModifyResult(
    AMAHUD* HUD,
    UMAModifyWidget* ModifyWidget,
    const TArray<AActor*>& Actors,
    const FString& LabelText,
    const FMASceneActionResult& Result) const
{
    if (!HUD)
    {
        return;
    }

    ApplyResult(HUD, Result);

    if (!Result.bSuccess)
    {
        ModifyWidgetStateCoordinator.ApplyActionResult(ModifyWidget, Actors, LabelText, Result);
        return;
    }

    ModifyWidgetStateCoordinator.ApplyActionResult(ModifyWidget, Actors, LabelText, Result);

    if (Result.bClearHighlightedActor)
    {
        HUD->ClearHighlightedActor();
    }
}
