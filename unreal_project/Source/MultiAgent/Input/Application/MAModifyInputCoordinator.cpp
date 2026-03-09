// Modify-mode input coordination and highlight state.

#include "MAModifyInputCoordinator.h"

#include "../MAPlayerController.h"

void FMAModifyInputCoordinator::HandleLeftClick(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    const bool bShiftPressed =
        PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift);

    FHitResult HitResult;
    if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
    {
        if (AActor* HitActor = HitResult.GetActor())
        {
            if (bShiftPressed)
            {
                AddToSelection(PlayerController, HitActor);
            }
            else
            {
                ClearAndSelect(PlayerController, HitActor);
            }
            return;
        }
    }

    ClearAllHighlights(PlayerController);
    BroadcastSelection(PlayerController);
}

void FMAModifyInputCoordinator::EnterMode(AMAPlayerController* PlayerController) const
{
    HUDInputAdapter.ShowModifyWidget(PlayerController);
}

void FMAModifyInputCoordinator::ExitMode(AMAPlayerController* PlayerController) const
{
    ClearAllHighlights(PlayerController);
    HUDInputAdapter.HideModifyWidget(PlayerController);
}

void FMAModifyInputCoordinator::ClearHighlight(AMAPlayerController* PlayerController) const
{
    ClearAllHighlights(PlayerController);
    BroadcastSelection(PlayerController);
}

void FMAModifyInputCoordinator::BroadcastSelection(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    PlayerController->OnModifyActorsSelected.Broadcast(PlayerController->HighlightedActors);

    if (PlayerController->HighlightedActors.Num() == 1)
    {
        PlayerController->OnModifyActorSelected.Broadcast(PlayerController->HighlightedActors[0]);
    }
    else if (PlayerController->HighlightedActors.Num() == 0)
    {
        PlayerController->OnModifyActorSelected.Broadcast(nullptr);
    }
}

void FMAModifyInputCoordinator::ClearAllHighlights(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    for (AActor* Actor : PlayerController->HighlightedActors)
    {
        if (Actor)
        {
            HighlightAdapter.SetActorTreeHighlight(Actor, false);
        }
    }

    PlayerController->HighlightedActors.Empty();
}

void FMAModifyInputCoordinator::AddToSelection(AMAPlayerController* PlayerController, AActor* Actor) const
{
    if (!PlayerController || !Actor)
    {
        return;
    }

    AActor* RootActor = HighlightAdapter.FindRootActor(Actor);
    if (PlayerController->HighlightedActors.Contains(RootActor))
    {
        RemoveFromSelection(PlayerController, RootActor);
        return;
    }

    PlayerController->HighlightedActors.Add(RootActor);
    HighlightAdapter.SetActorTreeHighlight(RootActor, true);
    BroadcastSelection(PlayerController);
}

void FMAModifyInputCoordinator::RemoveFromSelection(AMAPlayerController* PlayerController, AActor* Actor) const
{
    if (!PlayerController || !Actor)
    {
        return;
    }

    AActor* RootActor = HighlightAdapter.FindRootActor(Actor);
    if (PlayerController->HighlightedActors.Remove(RootActor) > 0)
    {
        HighlightAdapter.SetActorTreeHighlight(RootActor, false);
        BroadcastSelection(PlayerController);
    }
}

void FMAModifyInputCoordinator::ClearAndSelect(AMAPlayerController* PlayerController, AActor* Actor) const
{
    if (!PlayerController)
    {
        return;
    }

    ClearAllHighlights(PlayerController);
    if (!Actor)
    {
        BroadcastSelection(PlayerController);
        return;
    }

    AActor* RootActor = HighlightAdapter.FindRootActor(Actor);
    PlayerController->HighlightedActors.Add(RootActor);
    HighlightAdapter.SetActorTreeHighlight(RootActor, true);
    BroadcastSelection(PlayerController);
}
