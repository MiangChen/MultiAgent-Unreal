// Squad creation, formation, and control-group coordination.

#include "MASquadInputCoordinator.h"

#include "../../../Input/MAPlayerController.h"

void FMASquadInputCoordinator::HandleControlGroup(AMAPlayerController* PlayerController, int32 GroupIndex) const
{
    if (!PlayerController)
    {
        return;
    }

    const bool bCtrlPressed =
        PlayerController->IsInputKeyDown(EKeys::LeftControl) || PlayerController->IsInputKeyDown(EKeys::RightControl);

    if (bCtrlPressed)
    {
        RuntimeAdapter.SaveToControlGroup(PlayerController, GroupIndex);
        return;
    }

    RuntimeAdapter.SelectControlGroup(PlayerController, GroupIndex);
}

void FMASquadInputCoordinator::CycleFormation(AMAPlayerController* PlayerController) const
{
    RuntimeAdapter.CycleFormation(PlayerController);
}

FMAFeedback21Batch FMASquadInputCoordinator::CreateSquad(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    if (PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift))
    {
        return Feedback;
    }

    FString SquadName;
    int32 MemberCount = 0;
    if (!RuntimeAdapter.CreateSquadFromSelection(PlayerController, SquadName, MemberCount))
    {
        Feedback.AddMessage(TEXT("Select at least 2 agents to create squad (Q)"), EMAFeedback21MessageSeverity::Warning, 3.f);
        return Feedback;
    }

    Feedback.AddMessage(
        FString::Printf(TEXT("Created Squad '%s' with %d members"), *SquadName, MemberCount),
        EMAFeedback21MessageSeverity::Success,
        3.f);

    return Feedback;
}

FMAFeedback21Batch FMASquadInputCoordinator::DisbandSquad(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    if (!PlayerController->IsInputKeyDown(EKeys::LeftShift) && !PlayerController->IsInputKeyDown(EKeys::RightShift))
    {
        return Feedback;
    }

    const int32 DisbandedCount = RuntimeAdapter.DisbandSelectedSquads(PlayerController);
    if (DisbandedCount <= 0)
    {
        Feedback.AddMessage(TEXT("No squad to disband (Shift+Q)"), EMAFeedback21MessageSeverity::Warning, 3.f);
        return Feedback;
    }

    Feedback.AddMessage(
        FString::Printf(TEXT("Disbanded %d squad(s)"), DisbandedCount),
        EMAFeedback21MessageSeverity::Warning,
        3.f);
    return Feedback;
}
