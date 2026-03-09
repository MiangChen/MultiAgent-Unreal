// Scene-list selection resolution for edit-mode actors.

#include "MAHUDSceneListSelectionCoordinator.h"

#include "../MAHUD.h"
#include "../../../Core/Interaction/Feedback/MAFeedback21.h"

void FMAHUDSceneListSelectionCoordinator::HandleGoalClicked(AMAHUD* HUD, const FString& GoalId) const
{
    if (!HUD)
    {
        return;
    }

    if (HUD->RuntimeSelectGoalById(GoalId))
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

    if (HUD->RuntimeSelectZoneById(ZoneId))
    {
        return;
    }

    FMAFeedback21Batch Feedback;
    Feedback.AddMessage(
        FString::Printf(TEXT("Zone %s has no visualization Actor"), *ZoneId),
        EMAFeedback21MessageSeverity::Warning);

    HUD->ApplyInteractionFeedback(Feedback);
}
