// Command dispatch coordination for player input.

#include "MACommandInputCoordinator.h"

#include "../../../Input/MAPlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMACommandInputCoordinator, Log, All);

FMAFeedback21Batch FMACommandInputCoordinator::SendCommandToSelection(
    AMAPlayerController* PlayerController,
    EMACommand Command) const
{
    const FMAFeedback54 Feedback54 = RuntimeAdapter.SendCommandToSelection(PlayerController, Command);
    const FMAFeedback43 Feedback43 = FeedbackPipeline.ToFeedback43(Feedback54);
    const FMAFeedback32 Feedback32 = FeedbackPipeline.ToFeedback32(Feedback43);
    FMAFeedback21Batch Feedback21 = FeedbackPipeline.ToFeedback21(Feedback32);

    if (!Feedback54.bSuccess)
    {
        return Feedback21;
    }

    UE_LOG(LogMACommandInputCoordinator, Log, TEXT("Sent %s to %d selected agents"),
        *RuntimeAdapter.GetCommandDisplayName(Command),
        Feedback54.Count);
    return Feedback21;
}

FMAFeedback21Batch FMACommandInputCoordinator::TogglePauseExecution(AMAPlayerController* PlayerController) const
{
    const FMAFeedback54 Feedback54 = RuntimeAdapter.TogglePauseExecution(PlayerController);
    const FMAFeedback43 Feedback43 = FeedbackPipeline.ToFeedback43(Feedback54);
    const FMAFeedback32 Feedback32 = FeedbackPipeline.ToFeedback32(Feedback43);
    return FeedbackPipeline.ToFeedback21(Feedback32);
}
