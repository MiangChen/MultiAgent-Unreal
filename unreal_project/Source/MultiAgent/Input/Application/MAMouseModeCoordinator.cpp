// Mouse mode transition coordination.

#include "MAMouseModeCoordinator.h"

#include "../MAPlayerController.h"
#include "../../Core/Manager/MAEditModeManager.h"
#include "../../Core/Manager/MASelectionManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAMouseModeCoordinator, Log, All);

bool FMAMouseModeCoordinator::TransitionToMode(AMAPlayerController* PlayerController, EMAMouseMode NewMode) const
{
    if (!PlayerController)
    {
        return false;
    }

    const EMAMouseMode OldMode = PlayerController->CurrentMouseMode;
    if (OldMode == NewMode)
    {
        return false;
    }

    if (!CanEnterMode(PlayerController, NewMode))
    {
        return false;
    }

    CleanupDeploymentState(PlayerController, OldMode, NewMode);
    ApplyBaseInputSettings(PlayerController);
    HandleExit(PlayerController, OldMode, NewMode);

    PlayerController->PreviousMouseMode = OldMode;
    PlayerController->CurrentMouseMode = NewMode;

    HandleEnter(PlayerController, OldMode, NewMode);

    UE_LOG(LogMAMouseModeCoordinator, Log, TEXT("Mouse mode transition: %s -> %s"),
        *AMAPlayerController::MouseModeToString(OldMode),
        *AMAPlayerController::MouseModeToString(NewMode));

    return true;
}

FString FMAMouseModeCoordinator::BuildModeStatusText(EMAMouseMode Mode) const
{
    FString ExtraInfo;
    switch (Mode)
    {
    case EMAMouseMode::Edit:
        ExtraInfo = TEXT(" [Click to create POI, Shift+Click to select]");
        break;
    case EMAMouseMode::Modify:
        ExtraInfo = TEXT(" [Click to select Actor, Shift+Click for multi-select]");
        break;
    default:
        break;
    }

    return FString::Printf(TEXT("Mode: %s%s"),
        *AMAPlayerController::MouseModeToString(Mode),
        *ExtraInfo);
}

void FMAMouseModeCoordinator::ApplyBaseInputSettings(AMAPlayerController* PlayerController) const
{
    PlayerController->bShowMouseCursor = true;
    PlayerController->SetIgnoreLookInput(true);

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    PlayerController->SetInputMode(InputMode);
}

void FMAMouseModeCoordinator::CleanupDeploymentState(
    AMAPlayerController* PlayerController,
    EMAMouseMode OldMode,
    EMAMouseMode NewMode) const
{
    if (OldMode == EMAMouseMode::Deployment && NewMode != EMAMouseMode::Deployment)
    {
        if (PlayerController->SelectionManager && PlayerController->SelectionManager->IsBoxSelecting())
        {
            PlayerController->SelectionManager->CancelBoxSelect();
        }

        PlayerController->DeploymentInputCoordinator.ResetTransientState(PlayerController);
    }
}

bool FMAMouseModeCoordinator::CanEnterMode(AMAPlayerController* PlayerController, EMAMouseMode NewMode) const
{
    if (NewMode == EMAMouseMode::Edit
        && PlayerController->EditModeManager
        && !PlayerController->EditModeManager->IsEditModeAvailable())
    {
        UE_LOG(LogMAMouseModeCoordinator, Warning, TEXT("Edit Mode unavailable: source scene graph file not found"));
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
            TEXT("Edit Mode unavailable: source scene graph file not found"));
        return false;
    }

    return true;
}

void FMAMouseModeCoordinator::HandleExit(
    AMAPlayerController* PlayerController,
    EMAMouseMode OldMode,
    EMAMouseMode NewMode) const
{
    if (OldMode == EMAMouseMode::Modify && NewMode != EMAMouseMode::Modify)
    {
        PlayerController->ModifyInputCoordinator.ExitMode(PlayerController);
    }

    if (OldMode == EMAMouseMode::Edit && NewMode != EMAMouseMode::Edit)
    {
        PlayerController->EditInputCoordinator.ExitMode(PlayerController);
    }
}

void FMAMouseModeCoordinator::HandleEnter(
    AMAPlayerController* PlayerController,
    EMAMouseMode OldMode,
    EMAMouseMode NewMode) const
{
    if (NewMode == EMAMouseMode::Modify && OldMode != EMAMouseMode::Modify)
    {
        PlayerController->ModifyInputCoordinator.EnterMode(PlayerController);
    }

    if (NewMode == EMAMouseMode::Edit && OldMode != EMAMouseMode::Edit)
    {
        PlayerController->EditInputCoordinator.EnterMode(PlayerController);
    }
}
