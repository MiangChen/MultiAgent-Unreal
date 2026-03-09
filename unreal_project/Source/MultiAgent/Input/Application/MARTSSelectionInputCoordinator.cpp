// RTS selection box and middle-click navigation coordination.

#include "MARTSSelectionInputCoordinator.h"

#include "../MAPlayerController.h"
#include "../../Core/Manager/MASelectionManager.h"
#include "../../Agent/Character/MACharacter.h"
#include "../../Agent/Character/MAUAVCharacter.h"

void FMARTSSelectionInputCoordinator::HandleLeftClickStarted(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->SelectionManager)
    {
        return;
    }

    float MouseX = 0.f;
    float MouseY = 0.f;
    if (PlayerController->GetMousePosition(MouseX, MouseY))
    {
        PlayerController->SelectionManager->BeginBoxSelect(FVector2D(MouseX, MouseY));
    }
}

void FMARTSSelectionInputCoordinator::HandleLeftClickReleased(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->SelectionManager || !PlayerController->SelectionManager->IsBoxSelecting())
    {
        return;
    }

    float MouseX = 0.f;
    float MouseY = 0.f;
    if (PlayerController->GetMousePosition(MouseX, MouseY))
    {
        PlayerController->SelectionManager->UpdateBoxSelect(FVector2D(MouseX, MouseY));
    }

    const FVector2D Start = PlayerController->SelectionManager->GetBoxSelectStart();
    const FVector2D End = PlayerController->SelectionManager->GetBoxSelectEnd();
    const float BoxWidth = FMath::Abs(End.X - Start.X);
    const float BoxHeight = FMath::Abs(End.Y - Start.Y);

    if (BoxWidth < 10.f && BoxHeight < 10.f)
    {
        PlayerController->SelectionManager->CancelBoxSelect();

        FHitResult HitResult;
        if (PlayerController->GetHitResultUnderCursor(ECC_Pawn, false, HitResult))
        {
            if (AMACharacter* Agent = Cast<AMACharacter>(HitResult.GetActor()))
            {
                if (IsCtrlPressed(PlayerController))
                {
                    PlayerController->SelectionManager->ToggleSelection(Agent);
                }
                else
                {
                    PlayerController->SelectionManager->SelectAgent(Agent);
                }
            }
        }
        return;
    }

    PlayerController->SelectionManager->EndBoxSelect(PlayerController, IsCtrlPressed(PlayerController));
}

void FMARTSSelectionInputCoordinator::HandleMiddleClickNavigate(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->SelectionManager)
    {
        return;
    }

    const TArray<AMACharacter*> SelectedAgents = PlayerController->SelectionManager->GetSelectedAgents();
    if (SelectedAgents.IsEmpty())
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("No agents selected"));
        return;
    }

    if (HUDInputAdapter.IsMainUIVisible(PlayerController))
    {
        return;
    }

    FVector HitLocation;
    if (!PlayerController->GetMouseHitLocation(HitLocation))
    {
        return;
    }

    for (AMACharacter* Agent : SelectedAgents)
    {
        if (!Agent)
        {
            continue;
        }

        FVector Target = HitLocation;
        if (Agent->AgentType == EMAAgentType::UAV)
        {
            if (const AMAUAVCharacter* UAV = Cast<AMAUAVCharacter>(Agent); UAV && UAV->IsInAir())
            {
                Target.Z = Agent->GetActorLocation().Z;
            }
        }

        Agent->TryNavigateTo(Target);
    }
}

void FMARTSSelectionInputCoordinator::Tick(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->SelectionManager)
    {
        return;
    }

    if (PlayerController->SelectionManager->IsBoxSelecting())
    {
        float MouseX = 0.f;
        float MouseY = 0.f;
        if (PlayerController->GetMousePosition(MouseX, MouseY))
        {
            PlayerController->SelectionManager->UpdateBoxSelect(FVector2D(MouseX, MouseY));
        }
    }

    HUDInputAdapter.SyncSelectionBox(
        PlayerController,
        PlayerController->SelectionManager->IsBoxSelecting(),
        PlayerController->SelectionManager->GetBoxSelectStart(),
        PlayerController->SelectionManager->GetBoxSelectEnd());
}

bool FMARTSSelectionInputCoordinator::IsCtrlPressed(const AMAPlayerController* PlayerController) const
{
    return PlayerController
        && (PlayerController->IsInputKeyDown(EKeys::LeftControl)
            || PlayerController->IsInputKeyDown(EKeys::RightControl));
}
