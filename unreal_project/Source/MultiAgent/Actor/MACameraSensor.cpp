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
    
    // UnrealCV 风格设置 - 使用 LDR 最终颜色
    SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    SceneCaptureComponent->bAlwaysPersistRenderingState = true;
    SceneCaptureComponent->ShowFlags.SetPostProcessing(true);
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
    // UnrealCV 风格 - 使用 LDR 格式并设置正确的 Gamma
    RenderTarget->InitAutoFormat(Resolution.X, Resolution.Y);
    RenderTarget->TargetGamma = GEngine->GetDisplayGamma();  // 关键！匹配显示器 Gamma
    RenderTarget->UpdateResourceImmediate();
    
    SceneCaptureComponent->TextureTarget = RenderTarget;
    
    UE_LOG(LogTemp, Log, TEXT("[Camera] %s initialized with resolution %dx%d, FOV %.0f, Gamma %.2f"),
        *SensorName, Resolution.X, Resolution.Y, FOV, RenderTarget->TargetGamma);
}

bool AMACameraSensor::TakePhoto(const FString& FilePath)
{
    FString SavePath = FilePath;
    if (SavePath.IsEmpty())
    {
        FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
        SavePath = FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString::Printf(TEXT("%s_%s.png"), *SensorName, *Timestamp);
    }
    
    FString Directory = FPaths::GetPath(SavePath);
    IFileManager::Get().MakeDirectory(*Directory, true);
    
    // 方法1: 使用 SceneCaptureComponent (传感器视角)
    if (RenderTarget)
    {
        SceneCaptureComponent->CaptureScene();
        
        TArray<FColor> Pixels = CaptureFrame();
        if (Pixels.Num() > 0)
        {
            TArray64<uint8> CompressedData;
            FImageUtils::PNGCompressImageArray(Resolution.X, Resolution.Y, TArrayView64<const FColor>(Pixels), CompressedData);
            
            TArray<uint8> SaveData;
            SaveData.Append(CompressedData.GetData(), CompressedData.Num());
            
            if (FFileHelper::SaveArrayToFile(SaveData, *SavePath))
            {
                UE_LOG(LogTemp, Log, TEXT("[Camera] %s saved photo to %s"), *SensorName, *SavePath);
                return true;
            }
        }
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
    
    // UnrealCV 风格 - 直接读取 LDR 像素
    FReadSurfaceDataFlags ReadSurfaceDataFlags;
    ReadSurfaceDataFlags.SetLinearToGamma(false);
    RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadSurfaceDataFlags);
    
    return Pixels;
}
