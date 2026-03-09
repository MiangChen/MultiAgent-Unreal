// Mouse mode transition coordination.

#include "MAMouseModeCoordinator.h"

#include "../../../Input/MAPlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAMouseModeCoordinator, Log, All);

FMAFeedback21Batch FMAMouseModeCoordinator::TransitionToMode(AMAPlayerController* PlayerController, EMAMouseMode NewMode) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    const EMAMouseMode OldMode = PlayerController->MouseModeState.CurrentMode;
    if (OldMode == NewMode)
    {
        return Feedback;
    }

    if (!CanEnterMode(PlayerController, NewMode))
    {
        Feedback.AddMessage(
            TEXT("Edit Mode unavailable: source scene graph file not found"),
            EMAFeedback21MessageSeverity::Error,
            3.f);
        return Feedback;
    }

    CleanupDeploymentState(PlayerController, OldMode, NewMode);
    ApplyBaseInputSettings(PlayerController);
    Feedback.Append(HandleExit(PlayerController, OldMode, NewMode));

    PlayerController->MouseModeState.PreviousMode = OldMode;
    PlayerController->MouseModeState.CurrentMode = NewMode;

    Feedback.Append(HandleEnter(PlayerController, OldMode, NewMode));

    UE_LOG(LogMAMouseModeCoordinator, Log, TEXT("Mouse mode transition: %s -> %s"),
        *AMAPlayerController::MouseModeToString(OldMode),
        *AMAPlayerController::MouseModeToString(NewMode));

    return Feedback;
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
        if (RuntimeAdapter.IsSelectionBoxActive(PlayerController))
        {
            RuntimeAdapter.CancelSelectionBox(PlayerController);
        }

        PlayerController->DeploymentInputCoordinator.ResetTransientState(PlayerController);
    }
}

bool FMAMouseModeCoordinator::CanEnterMode(AMAPlayerController* PlayerController, EMAMouseMode NewMode) const
{
    if (NewMode == EMAMouseMode::Edit && !RuntimeAdapter.CanEnterEditMode(PlayerController))
    {
        UE_LOG(LogMAMouseModeCoordinator, Warning, TEXT("Edit Mode unavailable: source scene graph file not found"));
        return false;
    }

    return true;
}

FMAFeedback21Batch FMAMouseModeCoordinator::HandleExit(
    AMAPlayerController* PlayerController,
    EMAMouseMode OldMode,
    EMAMouseMode NewMode) const
{
    FMAFeedback21Batch Feedback;
    if (OldMode == EMAMouseMode::Modify && NewMode != EMAMouseMode::Modify)
    {
        Feedback.Append(PlayerController->ModifyInputCoordinator.ExitMode(PlayerController));
    }

    if (OldMode == EMAMouseMode::Edit && NewMode != EMAMouseMode::Edit)
    {
        Feedback.Append(PlayerController->EditInputCoordinator.ExitMode(PlayerController));
    }

    return Feedback;
}

FMAFeedback21Batch FMAMouseModeCoordinator::HandleEnter(
    AMAPlayerController* PlayerController,
    EMAMouseMode OldMode,
    EMAMouseMode NewMode) const
{
    FMAFeedback21Batch Feedback;
    if (NewMode == EMAMouseMode::Modify && OldMode != EMAMouseMode::Modify)
    {
        Feedback.Append(PlayerController->ModifyInputCoordinator.EnterMode(PlayerController));
    }

    if (NewMode == EMAMouseMode::Edit && OldMode != EMAMouseMode::Edit)
    {
        Feedback.Append(PlayerController->EditInputCoordinator.EnterMode(PlayerController));
    }

    return Feedback;
}
