// Camera switching, streaming, and right-drag orbit coordination.

#include "MACameraInputCoordinator.h"

#include "../../../Input/MAPlayerController.h"

void FMACameraInputCoordinator::HandleRightClickPressed(AMAPlayerController* PlayerController) const
{
    if (!PlayerController
        || PlayerController->RuntimeIsMouseOverPersistentUI()
        || PlayerController->RuntimeIsMainUIVisible())
    {
        return;
    }

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    PlayerController->SetInputMode(InputMode);

    PlayerController->bIsRightMouseRotating = true;

    float MouseX = 0.f;
    float MouseY = 0.f;
    if (PlayerController->GetMousePosition(MouseX, MouseY))
    {
        PlayerController->RightMouseStartPosition = FVector2D(MouseX, MouseY);
    }
}

void FMACameraInputCoordinator::HandleRightClickReleased(AMAPlayerController* PlayerController) const
{
    if (PlayerController)
    {
        PlayerController->bIsRightMouseRotating = false;
    }
}

void FMACameraInputCoordinator::Tick(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->bIsRightMouseRotating)
    {
        return;
    }

    float MouseX = 0.f;
    float MouseY = 0.f;
    if (!PlayerController->GetMousePosition(MouseX, MouseY))
    {
        return;
    }

    const FVector2D CurrentPos(MouseX, MouseY);
    const FVector2D Delta = CurrentPos - PlayerController->RightMouseStartPosition;
    if (Delta.Size() <= 2.f)
    {
        return;
    }

    if (APawn* ControlledPawn = PlayerController->GetPawn())
    {
        FRotator NewRotation = ControlledPawn->GetActorRotation();
        NewRotation.Yaw += Delta.X * PlayerController->CameraRotationSensitivity;
        NewRotation.Pitch = FMath::Clamp(
            NewRotation.Pitch - Delta.Y * PlayerController->CameraRotationSensitivity,
            -89.f,
            89.f);

        ControlledPawn->SetActorRotation(NewRotation);
        PlayerController->SetControlRotation(NewRotation);
    }

    PlayerController->RightMouseStartPosition = CurrentPos;
}

void FMACameraInputCoordinator::SwitchToNextCamera(AMAPlayerController* PlayerController) const
{
    PlayerController->RuntimeSwitchToNextCamera();
}

void FMACameraInputCoordinator::ReturnToSpectator(AMAPlayerController* PlayerController) const
{
    PlayerController->RuntimeReturnToSpectator();
}

FMAFeedback21Batch FMACameraInputCoordinator::TakePhoto(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    FString SensorName;
    if (PlayerController->RuntimeTakePhotoForCurrentCamera(SensorName))
    {
        Feedback.AddMessage(
            FString::Printf(TEXT("%s: Photo saved"), *SensorName),
            EMAFeedback21MessageSeverity::Success);
    }

    return Feedback;
}

FMAFeedback21Batch FMACameraInputCoordinator::ToggleTCPStream(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    const FMACameraStreamRuntimeResult Result = PlayerController->RuntimeToggleTCPStreamForCurrentCamera();
    if (Result.Event == EMACameraStreamEvent::None)
    {
        return Feedback;
    }

    if (Result.Event == EMACameraStreamEvent::Stopped)
    {
        Feedback.AddMessage(
            FString::Printf(TEXT("%s: TCP stopped"), *Result.SensorName),
            EMAFeedback21MessageSeverity::Warning,
            3.f);
        return Feedback;
    }

    Feedback.AddMessage(
        FString::Printf(TEXT("%s: TCP on port 9000"), *Result.SensorName),
        EMAFeedback21MessageSeverity::Success,
        5.f);

    return Feedback;
}
