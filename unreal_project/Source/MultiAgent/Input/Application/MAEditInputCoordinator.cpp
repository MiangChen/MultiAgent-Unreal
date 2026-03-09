// Edit-mode click and widget lifecycle coordination.

#include "MAEditInputCoordinator.h"

#include "../MAPlayerController.h"
#include "../../Core/Manager/MAEditModeManager.h"
#include "../../Environment/Utils/MAPointOfInterest.h"

void FMAEditInputCoordinator::HandleLeftClick(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->EditModeManager)
    {
        return;
    }

    if (HUDInputAdapter.IsMouseOverEditWidget(PlayerController))
    {
        return;
    }

    const bool bShiftPressed =
        PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift);

    if (bShiftPressed)
    {
        FHitResult HitResult;
        if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
        {
            if (AActor* HitActor = HitResult.GetActor())
            {
                if (AMAPointOfInterest* POI = Cast<AMAPointOfInterest>(HitActor))
                {
                    if (PlayerController->EditModeManager->GetSelectedPOIs().Contains(POI))
                    {
                        PlayerController->EditModeManager->DeselectObject(POI);
                    }
                    else
                    {
                        PlayerController->EditModeManager->SelectPOI(POI);
                    }
                    return;
                }

                if (PlayerController->EditModeManager->GetSelectedActor() == HitActor)
                {
                    PlayerController->EditModeManager->DeselectObject(HitActor);
                }
                else
                {
                    PlayerController->EditModeManager->SelectActor(HitActor);
                }
                return;
            }
        }

        PlayerController->EditModeManager->ClearSelection();
        return;
    }

    FVector HitLocation;
    if (PlayerController->GetMouseHitLocation(HitLocation))
    {
        if (AMAPointOfInterest* NewPOI = PlayerController->EditModeManager->CreatePOI(HitLocation))
        {
            UE_LOG(LogTemp, Log, TEXT("[EditInput] Created POI at %s"), *HitLocation.ToString());
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
                FString::Printf(TEXT("POI: (%.0f, %.0f, %.0f)"), HitLocation.X, HitLocation.Y, HitLocation.Z));
        }
    }
}

void FMAEditInputCoordinator::EnterMode(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->EditModeManager)
    {
        return;
    }

    HUDInputAdapter.ShowEditWidget(PlayerController);
}

void FMAEditInputCoordinator::ExitMode(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    if (PlayerController->EditModeManager)
    {
        PlayerController->EditModeManager->ClearSelection();
        PlayerController->EditModeManager->DestroyAllPOIs();
    }

    HUDInputAdapter.HideEditWidget(PlayerController);
}
