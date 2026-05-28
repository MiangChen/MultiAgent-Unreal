// Squad creation, formation, and control-group coordination.

#include "MASquadInputCoordinator.h"

#include "../../Squad/Feedback/MASquadFeedback.h"
#include "../../../Input/MAPlayerController.h"

namespace
{
EMAFeedback21MessageSeverity ToInteractionSeverity(const EMASquadFeedbackSeverity Severity)
{
    switch (Severity)
    {
    case EMASquadFeedbackSeverity::Success:
        return EMAFeedback21MessageSeverity::Success;
    case EMASquadFeedbackSeverity::Warning:
        return EMAFeedback21MessageSeverity::Warning;
    case EMASquadFeedbackSeverity::Error:
        return EMAFeedback21MessageSeverity::Error;
    case EMASquadFeedbackSeverity::Info:
    default:
        return EMAFeedback21MessageSeverity::Info;
    }
}

FMAFeedback21Batch BuildSquadFeedback(const FMASquadOperationFeedback& Feedback)
{
    FMAFeedback21Batch Batch;
    Batch.AddMessage(Feedback.Message, ToInteractionSeverity(Feedback.Severity), 3.f);
    return Batch;
}
}

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
        PlayerController->RuntimeSaveToControlGroup(GroupIndex);
        return;
    }

    PlayerController->RuntimeSelectControlGroup(GroupIndex);
}

FMAFeedback21Batch FMASquadInputCoordinator::CycleFormation(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return {};
    }

    return BuildSquadFeedback(PlayerController->RuntimeCycleFormation());
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

    return BuildSquadFeedback(PlayerController->RuntimeCreateSquadFromSelection());
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

    return BuildSquadFeedback(PlayerController->RuntimeDisbandSelectedSquads());
}
