// Edit-mode click and widget lifecycle coordination.

#include "MAEditInputCoordinator.h"

#include "../../../Input/MAPlayerController.h"

FMAFeedback21Batch FMAEditInputCoordinator::HandleLeftClick(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    const bool bShiftPressed =
        PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift);

    if (bShiftPressed)
    {
        FHitResult HitResult;
        if (RuntimeAdapter.ResolveCursorHit(PlayerController, ECC_Visibility, HitResult))
        {
            if (AActor* HitActor = HitResult.GetActor())
            {
                RuntimeAdapter.ToggleEditSelection(PlayerController, HitActor);
                return Feedback;
            }
        }

        RuntimeAdapter.ClearEditSelection(PlayerController);
        return Feedback;
    }

    FVector HitLocation;
    if (RuntimeAdapter.ResolveMouseHitLocation(PlayerController, HitLocation))
    {
        if (RuntimeAdapter.CreatePOI(PlayerController, HitLocation))
        {
            UE_LOG(LogTemp, Log, TEXT("[EditInput] Created POI at %s"), *HitLocation.ToString());
            Feedback.AddMessage(
                FString::Printf(TEXT("POI: (%.0f, %.0f, %.0f)"), HitLocation.X, HitLocation.Y, HitLocation.Z),
                EMAFeedback21MessageSeverity::Success,
                3.f);
        }
    }

    return Feedback;
}

FMAFeedback21Batch FMAEditInputCoordinator::EnterMode(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    Feedback.AddHUDAction(EMAFeedback21HUDAction::ShowEditWidget);
    return Feedback;
}

FMAFeedback21Batch FMAEditInputCoordinator::ExitMode(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    RuntimeAdapter.ClearEditSelection(PlayerController);
    RuntimeAdapter.DestroyAllPOIs(PlayerController);

    Feedback.AddHUDAction(EMAFeedback21HUDAction::HideEditWidget);
    return Feedback;
}
