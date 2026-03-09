// RTS selection box and middle-click navigation coordination.

#include "MARTSSelectionInputCoordinator.h"

#include "../../../Input/MAPlayerController.h"

void FMARTSSelectionInputCoordinator::HandleLeftClickStarted(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    float MouseX = 0.f;
    float MouseY = 0.f;
    if (PlayerController->GetMousePosition(MouseX, MouseY))
    {
        RuntimeAdapter.BeginSelectionBox(PlayerController, FVector2D(MouseX, MouseY));
    }
}

void FMARTSSelectionInputCoordinator::HandleLeftClickReleased(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !RuntimeAdapter.IsSelectionBoxActive(PlayerController))
    {
        return;
    }

    float MouseX = 0.f;
    float MouseY = 0.f;
    if (PlayerController->GetMousePosition(MouseX, MouseY))
    {
        RuntimeAdapter.UpdateSelectionBox(PlayerController, FVector2D(MouseX, MouseY));
    }

    const FVector2D Start = RuntimeAdapter.GetSelectionBoxStart(PlayerController);
    const FVector2D End = RuntimeAdapter.GetSelectionBoxEnd(PlayerController);
    const float BoxWidth = FMath::Abs(End.X - Start.X);
    const float BoxHeight = FMath::Abs(End.Y - Start.Y);

    if (BoxWidth < 10.f && BoxHeight < 10.f)
    {
        RuntimeAdapter.CancelSelectionBox(PlayerController);

        if (AMACharacter* Agent = RuntimeAdapter.ResolveClickedAgent(PlayerController))
        {
            if (IsCtrlPressed(PlayerController))
            {
                RuntimeAdapter.ToggleSelection(PlayerController, Agent);
            }
            else
            {
                RuntimeAdapter.SelectAgent(PlayerController, Agent);
            }
        }
        return;
    }

    RuntimeAdapter.EndSelectionBox(PlayerController, IsCtrlPressed(PlayerController));
}

FMAFeedback21Batch FMARTSSelectionInputCoordinator::HandleMiddleClickNavigate(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    const TArray<AMACharacter*> SelectedAgents = RuntimeAdapter.GetSelectedAgents(PlayerController);
    if (SelectedAgents.IsEmpty())
    {
        Feedback.AddMessage(TEXT("No agents selected"), EMAFeedback21MessageSeverity::Warning);
        return Feedback;
    }

    FVector HitLocation;
    if (!PlayerController->GetMouseHitLocation(HitLocation))
    {
        return Feedback;
    }

    RuntimeAdapter.NavigateSelectedAgentsToLocation(PlayerController, HitLocation);

    return Feedback;
}

FMAFeedback21Batch FMARTSSelectionInputCoordinator::Tick(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    if (RuntimeAdapter.IsSelectionBoxActive(PlayerController))
    {
        float MouseX = 0.f;
        float MouseY = 0.f;
        if (PlayerController->GetMousePosition(MouseX, MouseY))
        {
            RuntimeAdapter.UpdateSelectionBox(PlayerController, FVector2D(MouseX, MouseY));
        }
    }

    Feedback.SetSelectionBox(
        RuntimeAdapter.IsSelectionBoxActive(PlayerController),
        RuntimeAdapter.GetSelectionBoxStart(PlayerController),
        RuntimeAdapter.GetSelectionBoxEnd(PlayerController));

    return Feedback;
}

bool FMARTSSelectionInputCoordinator::IsCtrlPressed(const AMAPlayerController* PlayerController) const
{
    return PlayerController
        && (PlayerController->IsInputKeyDown(EKeys::LeftControl)
            || PlayerController->IsInputKeyDown(EKeys::RightControl));
}
