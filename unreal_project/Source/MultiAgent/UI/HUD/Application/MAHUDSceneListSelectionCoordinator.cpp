// Scene-list selection resolution for edit-mode actors.

#include "MAHUDSceneListSelectionCoordinator.h"

#include "../MAHUD.h"
#include "../../../Core/Interaction/Feedback/MAFeedback21.h"
#include "../../../Core/Interaction/Infrastructure/MAFeedback21Applier.h"

void FMAHUDSceneListSelectionCoordinator::HandleGoalClicked(AMAHUD* HUD, const FString& GoalId) const
{
    if (!HUD)
    {
        return;
    }

    if (RuntimeAdapter.SelectGoalById(HUD, GoalId))
    {
        return;
    }
}

void FMAHUDSceneListSelectionCoordinator::HandleZoneClicked(AMAHUD* HUD, const FString& ZoneId) const
{
    if (!HUD)
    {
        return;
    }

    if (RuntimeAdapter.SelectZoneById(HUD, ZoneId))
    {
        return;
    }

    FMAFeedback21Batch Feedback;
    Feedback.AddMessage(
        FString::Printf(TEXT("Zone %s has no visualization Actor"), *ZoneId),
        EMAFeedback21MessageSeverity::Warning);

    FMAFeedback21Applier Feedback21Applier;
    Feedback21Applier.ApplyToHUD(HUD, Feedback);
}
