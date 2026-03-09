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
        PlayerController->RuntimeBeginSelectionBox(FVector2D(MouseX, MouseY));
    }
}

void FMARTSSelectionInputCoordinator::HandleLeftClickReleased(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->RuntimeIsSelectionBoxActive())
    {
        return;
    }

    float MouseX = 0.f;
    float MouseY = 0.f;
    if (PlayerController->GetMousePosition(MouseX, MouseY))
    {
        PlayerController->RuntimeUpdateSelectionBox(FVector2D(MouseX, MouseY));
    }

    const FVector2D Start = PlayerController->RuntimeGetSelectionBoxStart();
    const FVector2D End = PlayerController->RuntimeGetSelectionBoxEnd();
    const float BoxWidth = FMath::Abs(End.X - Start.X);
    const float BoxHeight = FMath::Abs(End.Y - Start.Y);

    if (BoxWidth < 10.f && BoxHeight < 10.f)
    {
        PlayerController->RuntimeCancelSelectionBox();

        if (AMACharacter* Agent = PlayerController->RuntimeResolveClickedAgent())
        {
            if (IsCtrlPressed(PlayerController))
            {
                PlayerController->RuntimeToggleSelection(Agent);
            }
            else
            {
                PlayerController->RuntimeSelectAgent(Agent);
            }
        }
        return;
    }

    PlayerController->RuntimeEndSelectionBox(IsCtrlPressed(PlayerController));
}

FMAFeedback21Batch FMARTSSelectionInputCoordinator::HandleMiddleClickNavigate(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    const TArray<AMACharacter*> SelectedAgents = PlayerController->RuntimeGetSelectedAgents();
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

    PlayerController->RuntimeNavigateSelectedAgentsToLocation(HitLocation);

    return Feedback;
}

FMAFeedback21Batch FMARTSSelectionInputCoordinator::Tick(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    if (PlayerController->RuntimeIsSelectionBoxActive())
    {
        float MouseX = 0.f;
        float MouseY = 0.f;
        if (PlayerController->GetMousePosition(MouseX, MouseY))
        {
            PlayerController->RuntimeUpdateSelectionBox(FVector2D(MouseX, MouseY));
        }
    }

    Feedback.SetSelectionBox(
        PlayerController->RuntimeIsSelectionBoxActive(),
        PlayerController->RuntimeGetSelectionBoxStart(),
        PlayerController->RuntimeGetSelectionBoxEnd());

    return Feedback;
}

bool FMARTSSelectionInputCoordinator::IsCtrlPressed(const AMAPlayerController* PlayerController) const
{
    return PlayerController
        && (PlayerController->IsInputKeyDown(EKeys::LeftControl)
            || PlayerController->IsInputKeyDown(EKeys::RightControl));
}
