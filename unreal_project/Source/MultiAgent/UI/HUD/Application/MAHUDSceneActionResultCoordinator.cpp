#include "MAHUDSceneActionResultCoordinator.h"

#include "../Runtime/MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../SceneEditing/Presentation/MAModifyWidget.h"
#include "../../SceneEditing/Presentation/MASceneListWidget.h"

namespace
{
FMAFeedback21Batch BuildSceneActionFeedback(const FMASceneActionResult& Result)
{
    FMAFeedback21Batch Feedback;
    Feedback.AddMessage(
        Result.Message,
        !Result.bSuccess && !Result.bIsWarning
            ? EMAFeedback21MessageSeverity::Error
            : (Result.bIsWarning ? EMAFeedback21MessageSeverity::Warning : EMAFeedback21MessageSeverity::Success));

    if (Result.bRefreshSceneList)
    {
        Feedback.AddHUDAction(EMAFeedback21HUDAction::RefreshSceneList);
    }

    if (Result.bReloadSceneVisualization)
    {
        Feedback.AddHUDAction(EMAFeedback21HUDAction::ReloadSceneVisualization);
    }

    if (Result.bRefreshSelection)
    {
        Feedback.AddHUDAction(EMAFeedback21HUDAction::RefreshSelection);
    }

    if (Result.bClearHighlightedActor)
    {
        Feedback.AddHUDAction(EMAFeedback21HUDAction::ClearHighlightedActor);
    }

    return Feedback;
}
}

void FMAHUDSceneActionResultCoordinator::ApplyFeedback(AMAHUD* HUD, const FMAFeedback21Batch& Feedback) const
{
    if (HUD)
    {
        HUD->ApplyInteractionFeedback(Feedback);
    }
}

void FMAHUDSceneActionResultCoordinator::ApplyResult(AMAHUD* HUD, const FMASceneActionResult& Result) const
{
    ApplyFeedback(HUD, BuildSceneActionFeedback(Result));
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

    ApplyFeedback(HUD, BuildSceneActionFeedback(Result));

    if (!Result.bSuccess)
    {
        ModifyWidgetStateCoordinator.ApplyActionResult(ModifyWidget, Actors, LabelText, Result);
        return;
    }

    ModifyWidgetStateCoordinator.ApplyActionResult(ModifyWidget, Actors, LabelText, Result);
}
