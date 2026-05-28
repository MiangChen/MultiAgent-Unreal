#include "MACameraSensorComponent.h"

#include "Agent/Sensing/Application/MASensingUseCases.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "RenderingThread.h"

bool UMACameraSensorComponent::TakePhoto(const FString& FilePath)
{
    const FMASensingOperationFeedback PathFeedback =
        FMASensingUseCases::BuildPhotoSaveTarget(SensorName, FilePath);
    const FString SavePath = PathFeedback.ResolvedPath;

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(SavePath), true);

    if (RenderTarget)
    {
        const TArray<FColor> Pixels = CaptureFrame();
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

TArray<FColor> UMACameraSensorComponent::CaptureFrame()
{
    TArray<FColor> Pixels;
    if (!RenderTarget || !SceneCaptureComponent || !SceneCaptureComponent->TextureTarget)
    {
        return Pixels;
    }

    SceneCaptureComponent->CaptureScene();
    FlushRenderingCommands();

    if (FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource())
    {
        FReadSurfaceDataFlags ReadSurfaceDataFlags;
        ReadSurfaceDataFlags.SetLinearToGamma(false);
        RenderTargetResource->ReadPixels(Pixels, ReadSurfaceDataFlags);
    }

    return Pixels;
}

TArray<uint8> UMACameraSensorComponent::GetFrameAsJPEG(const int32 Quality)
{
    TArray<uint8> JPEGData;
    const TArray<FColor> Pixels = CaptureFrame();
    if (Pixels.Num() == 0)
    {
        return JPEGData;
    }

    IImageWrapperModule& ImageWrapperModule =
        FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    const TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
    if (!ImageWrapper.IsValid())
    {
        return JPEGData;
    }

    ImageWrapper->SetRaw(
        Pixels.GetData(),
        Pixels.Num() * sizeof(FColor),
        Resolution.X,
        Resolution.Y,
        ERGBFormat::BGRA,
        8);
    const TArray64<uint8> CompressedData = ImageWrapper->GetCompressed(Quality);
    JPEGData.Append(CompressedData.GetData(), CompressedData.Num());
    return JPEGData;
}
