#include "MACameraSensorComponent.h"
#include "Agent/Sensing/Bootstrap/MASensingBootstrap.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"

UMACameraSensorComponent::UMACameraSensorComponent()
{
    SensorType = EMASensorType::Camera;
    SensorName = TEXT("Camera");
}

void UMACameraSensorComponent::OnRegister()
{
    Super::OnRegister();

    if (!CameraComponent)
    {
        CameraComponent = NewObject<UCameraComponent>(GetOwner(), NAME_None, RF_Transactional);
        CameraComponent->SetupAttachment(this);
        CameraComponent->SetRelativeLocation(FVector::ZeroVector);
        CameraComponent->FieldOfView = FOV;
        CameraComponent->RegisterComponent();
    }
    
    if (!SceneCaptureComponent)
    {
        SceneCaptureComponent = NewObject<USceneCaptureComponent2D>(GetOwner(), NAME_None, RF_Transactional);
        SceneCaptureComponent->SetupAttachment(CameraComponent);
        SceneCaptureComponent->bCaptureEveryFrame = false;
        SceneCaptureComponent->bCaptureOnMovement = false;
        SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
        SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
        SceneCaptureComponent->bAlwaysPersistRenderingState = true;
        SceneCaptureComponent->ShowFlags.SetPostProcessing(true);
        SceneCaptureComponent->RegisterComponent();
    }
}

void UMACameraSensorComponent::BeginDestroy()
{
    StopTCPStream();
    Super::BeginDestroy();
}

void UMACameraSensorComponent::BeginPlay()
{
    Super::BeginPlay();

    FMASensingBootstrap::InitializeCameraSensor(*this);
}

void UMACameraSensorComponent::InitializeRenderTarget()
{
    RenderTarget = NewObject<UTextureRenderTarget2D>(this);
    RenderTarget->InitAutoFormat(Resolution.X, Resolution.Y);
    RenderTarget->TargetGamma = GEngine->GetDisplayGamma();
    RenderTarget->UpdateResourceImmediate();
    SceneCaptureComponent->TextureTarget = RenderTarget;
}
