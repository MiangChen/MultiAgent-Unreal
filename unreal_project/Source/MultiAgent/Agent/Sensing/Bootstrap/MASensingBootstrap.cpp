#include "MASensingBootstrap.h"
#include "Agent/Sensing/Runtime/MACameraSensorComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"

void FMASensingBootstrap::InitializeCameraSensor(UMACameraSensorComponent& CameraSensor)
{
    if (UCameraComponent* CameraComponent = CameraSensor.GetCameraComponent())
    {
        CameraComponent->FieldOfView = CameraSensor.FOV;
    }
    if (USceneCaptureComponent2D* SceneCaptureComponent = CameraSensor.GetSceneCaptureComponent())
    {
        SceneCaptureComponent->FOVAngle = CameraSensor.FOV;
    }
    CameraSensor.InitializeRenderTarget();
}
