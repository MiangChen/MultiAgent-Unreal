// Squad creation, formation, and control-group coordination.

#include "MASquadInputCoordinator.h"

#include "../MAPlayerController.h"
#include "../../Core/MASquad.h"
#include "../../Core/Manager/MASquadManager.h"
#include "../../Core/Manager/MASelectionManager.h"
#include "../../Agent/Character/MACharacter.h"

void FMASquadInputCoordinator::HandleControlGroup(AMAPlayerController* PlayerController, int32 GroupIndex) const
{
    if (!PlayerController || !PlayerController->SelectionManager)
    {
        return;
    }

    const bool bCtrlPressed =
        PlayerController->IsInputKeyDown(EKeys::LeftControl) || PlayerController->IsInputKeyDown(EKeys::RightControl);

    if (bCtrlPressed)
    {
        PlayerController->SelectionManager->SaveToControlGroup(GroupIndex);
        return;
    }

    PlayerController->SelectionManager->SelectControlGroup(GroupIndex);
}

void FMASquadInputCoordinator::CycleFormation(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->SquadManager)
    {
        return;
    }

    if (UMASquad* Squad = PlayerController->SquadManager->GetOrCreateDefaultSquad())
    {
        PlayerController->SquadManager->CycleFormation(Squad);
    }
}

void FMASquadInputCoordinator::CreateSquad(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    if (PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift))
    {
        return;
    }

    if (!PlayerController->SelectionManager || !PlayerController->SquadManager)
    {
        return;
    }

    const TArray<AMACharacter*> Selected = PlayerController->SelectionManager->GetSelectedAgents();
    if (Selected.Num() < 2)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
            TEXT("Select at least 2 agents to create squad (Q)"));
        return;
    }

    if (UMASquad* Squad = PlayerController->SquadManager->CreateSquad(Selected, Selected[0]))
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
            FString::Printf(TEXT("Created Squad '%s' with %d members"),
                *Squad->SquadName, Squad->GetMemberCount()));
    }
}

void FMASquadInputCoordinator::DisbandSquad(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    if (!PlayerController->IsInputKeyDown(EKeys::LeftShift) && !PlayerController->IsInputKeyDown(EKeys::RightShift))
    {
        return;
    }

    if (!PlayerController->SelectionManager || !PlayerController->SquadManager)
    {
        return;
    }

    const TArray<AMACharacter*> Selected = PlayerController->SelectionManager->GetSelectedAgents();
    TSet<UMASquad*> SquadsToDisband;
    for (AMACharacter* Agent : Selected)
    {
        if (Agent && Agent->CurrentSquad)
        {
            SquadsToDisband.Add(Agent->CurrentSquad);
        }
    }

    if (SquadsToDisband.IsEmpty())
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
            TEXT("No squad to disband (Shift+Q)"));
        return;
    }

    int32 DisbandedCount = 0;
    for (UMASquad* Squad : SquadsToDisband)
    {
        if (PlayerController->SquadManager->DisbandSquad(Squad))
        {
            DisbandedCount++;
        }
    }

    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
        FString::Printf(TEXT("Disbanded %d squad(s)"), DisbandedCount));
}
