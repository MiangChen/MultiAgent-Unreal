// Camera switching, streaming, and right-drag orbit coordination.

#include "MACameraInputCoordinator.h"

#include "../MAPlayerController.h"
#include "../../Core/Manager/MAAgentManager.h"
#include "../../Core/Manager/MAViewportManager.h"
#include "../../Agent/Character/MACharacter.h"
#include "../../Agent/Component/Sensor/MACameraSensorComponent.h"

void FMACameraInputCoordinator::HandleRightClickPressed(AMAPlayerController* PlayerController) const
{
    if (!PlayerController
        || HUDInputAdapter.IsMouseOverPersistentUI(PlayerController)
        || HUDInputAdapter.IsMainUIVisible(PlayerController))
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
    if (PlayerController && PlayerController->ViewportManager)
    {
        PlayerController->ViewportManager->SwitchToNextCamera(PlayerController);
    }
}

void FMACameraInputCoordinator::ReturnToSpectator(AMAPlayerController* PlayerController) const
{
    if (PlayerController && PlayerController->ViewportManager)
    {
        PlayerController->ViewportManager->ReturnToSpectator(PlayerController);
    }
}

void FMACameraInputCoordinator::TakePhoto(AMAPlayerController* PlayerController) const
{
    if (UMACameraSensorComponent* Camera = ResolveCurrentCamera(PlayerController))
    {
        if (Camera->TakePhoto())
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
                FString::Printf(TEXT("%s: Photo saved"), *Camera->SensorName));
        }
    }
}

void FMACameraInputCoordinator::ToggleTCPStream(AMAPlayerController* PlayerController) const
{
    UMACameraSensorComponent* Camera = ResolveCurrentCamera(PlayerController);
    if (!Camera)
    {
        return;
    }

    if (PlayerController && PlayerController->AgentManager)
    {
        for (AMACharacter* Agent : PlayerController->AgentManager->GetAllAgents())
        {
            if (!Agent)
            {
                continue;
            }

            if (UMACameraSensorComponent* OtherCamera = Agent->GetCameraSensor())
            {
                if (OtherCamera != Camera && OtherCamera->bIsStreaming)
                {
                    OtherCamera->StopTCPStream();
                }
            }
        }
    }

    if (Camera->bIsStreaming)
    {
        Camera->StopTCPStream();
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
            FString::Printf(TEXT("%s: TCP stopped"), *Camera->SensorName));
        return;
    }

    if (Camera->StartTCPStream(9000, Camera->StreamFPS))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
            FString::Printf(TEXT("%s: TCP on port 9000"), *Camera->SensorName));
    }
}

UMACameraSensorComponent* FMACameraInputCoordinator::ResolveCurrentCamera(const AMAPlayerController* PlayerController) const
{
    return PlayerController && PlayerController->ViewportManager
        ? PlayerController->ViewportManager->GetCurrentCamera()
        : nullptr;
}
