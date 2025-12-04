// MACameraSensor.cpp

#include "MACameraSensor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"

AMACameraSensor::AMACameraSensor()
{
    SensorType = EMASensorType::Camera;
    SensorName = TEXT("Camera");
    
    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(RootSceneComponent);
    CameraComponent->SetRelativeLocation(FVector::ZeroVector);
    CameraComponent->FieldOfView = FOV;
    
    SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
    SceneCaptureComponent->SetupAttachment(CameraComponent);
    SceneCaptureComponent->bCaptureEveryFrame = false;
    SceneCaptureComponent->bCaptureOnMovement = false;
}

void AMACameraSensor::BeginPlay()
{
    Super::BeginPlay();
    
    CameraComponent->FieldOfView = FOV;
    SceneCaptureComponent->FOVAngle = FOV;
    
    InitializeRenderTarget();
}

void AMACameraSensor::InitializeRenderTarget()
{
    RenderTarget = NewObject<UTextureRenderTarget2D>(this);
    RenderTarget->InitCustomFormat(Resolution.X, Resolution.Y, PF_B8G8R8A8, false);
    RenderTarget->UpdateResourceImmediate();
    
    SceneCaptureComponent->TextureTarget = RenderTarget;
    
    UE_LOG(LogTemp, Log, TEXT("[Camera] %s initialized with resolution %dx%d, FOV %.0f"),
        *SensorName, Resolution.X, Resolution.Y, FOV);
}

bool AMACameraSensor::TakePhoto(const FString& FilePath)
{
    if (!RenderTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Camera] TakePhoto failed: RenderTarget not initialized"));
        return false;
    }
    
    SceneCaptureComponent->CaptureScene();
    
    TArray<FColor> Pixels = CaptureFrame();
    if (Pixels.Num() == 0)
    {
        return false;
    }
    
    FString SavePath = FilePath;
    if (SavePath.IsEmpty())
    {
        FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
        SavePath = FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString::Printf(TEXT("%s_%s.png"), *SensorName, *Timestamp);
    }
    
    FString Directory = FPaths::GetPath(SavePath);
    IFileManager::Get().MakeDirectory(*Directory, true);
    
    TArray64<uint8> CompressedData;
    FImageUtils::PNGCompressImageArray(Resolution.X, Resolution.Y, TArrayView64<const FColor>(Pixels), CompressedData);
    
    TArray<uint8> SaveData;
    SaveData.Append(CompressedData.GetData(), CompressedData.Num());
    
    if (FFileHelper::SaveArrayToFile(SaveData, *SavePath))
    {
        UE_LOG(LogTemp, Log, TEXT("[Camera] %s saved photo to %s"), *SensorName, *SavePath);
        return true;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[Camera] Failed to save photo to %s"), *SavePath);
    return false;
}

TArray<FColor> AMACameraSensor::CaptureFrame()
{
    TArray<FColor> Pixels;
    
    if (!RenderTarget)
    {
        return Pixels;
    }
    
    SceneCaptureComponent->CaptureScene();
    
    FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();
    if (Resource)
    {
        Resource->ReadPixels(Pixels);
    }
    
    return Pixels;
}
