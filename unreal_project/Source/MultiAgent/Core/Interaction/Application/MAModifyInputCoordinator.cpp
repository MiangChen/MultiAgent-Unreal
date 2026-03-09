// Modify-mode input coordination and highlight state.

#include "MAModifyInputCoordinator.h"

#include "../../../Input/MAPlayerController.h"

void FMAModifyInputCoordinator::HandleLeftClick(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    const bool bShiftPressed =
        PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift);

    FHitResult HitResult;
    if (RuntimeAdapter.ResolveCursorHit(PlayerController, ECC_Visibility, HitResult))
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

FMAFeedback21Batch FMAModifyInputCoordinator::EnterMode(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (PlayerController)
    {
        Feedback.AddHUDAction(EMAFeedback21HUDAction::ShowModifyWidget);
    }
    return Feedback;
}

FMAFeedback21Batch FMAModifyInputCoordinator::ExitMode(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    ClearAllHighlights(PlayerController);
    if (PlayerController)
    {
        Feedback.AddHUDAction(EMAFeedback21HUDAction::HideModifyWidget);
    }
    return Feedback;
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

    const TArray<AActor*> HighlightedActors = PlayerController->ModifySelectionState.ToRawArray();
    PlayerController->OnModifyActorsSelected.Broadcast(HighlightedActors);

    if (HighlightedActors.Num() == 1)
    {
        PlayerController->OnModifyActorSelected.Broadcast(HighlightedActors[0]);
    }
    else if (HighlightedActors.Num() == 0)
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

    for (AActor* Actor : PlayerController->ModifySelectionState.ToRawArray())
    {
        if (Actor)
        {
            HighlightAdapter.SetActorTreeHighlight(Actor, false);
        }
    }

    PlayerController->ModifySelectionState.Reset();
}

void FMAModifyInputCoordinator::AddToSelection(AMAPlayerController* PlayerController, AActor* Actor) const
{
    if (!PlayerController || !Actor)
    {
        return;
    }

    AActor* RootActor = HighlightAdapter.FindRootActor(Actor);
    if (PlayerController->ModifySelectionState.Contains(RootActor))
    {
        RemoveFromSelection(PlayerController, RootActor);
        return;
    }

    PlayerController->ModifySelectionState.Add(RootActor);
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
    if (PlayerController->ModifySelectionState.Contains(RootActor))
    {
        PlayerController->ModifySelectionState.Remove(RootActor);
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
    PlayerController->ModifySelectionState.Add(RootActor);
    HighlightAdapter.SetActorTreeHighlight(RootActor, true);
    BroadcastSelection(PlayerController);
}
