// MAViewportRecorder.cpp

#include "MAViewportRecorder.h"
#include "Engine/GameViewportClient.h"
#include "Slate/SceneViewport.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SViewport.h"
#include "RenderingThread.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAViewportRecorder, Log, All);

void UMAViewportRecorder::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogMAViewportRecorder, Log, TEXT("ViewportRecorder initialized"));
}

void UMAViewportRecorder::Deinitialize()
{
    if (bIsRecording)
    {
        StopRecording();
    }
    Super::Deinitialize();
}

void UMAViewportRecorder::StartRecording(float FPS, float Scale)
{
    if (bIsRecording)
    {
        UE_LOG(LogMAViewportRecorder, Warning, TEXT("Already recording"));
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogMAViewportRecorder, Error, TEXT("No world available"));
        return;
    }
    
    // 设置参数
    RecordingFPS = FPS;
    ResolutionScale = Scale;
    
    // 创建录制目录
    FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
    RecordingDirectory = FPaths::ProjectSavedDir() / TEXT("ViewportRecordings") / Timestamp;
    IFileManager::Get().MakeDirectory(*RecordingDirectory, true);
    
    RecordingFrameIndex = 0;
    bIsRecording = true;
    bHasPendingFrame = false;
    
    // 使用定时器按帧率捕获
    float Interval = 1.0f / RecordingFPS;
    World->GetTimerManager().SetTimer(RecordTimerHandle, this, &UMAViewportRecorder::OnRecordTick, Interval, true);
    
    UE_LOG(LogMAViewportRecorder, Log, TEXT("Started viewport recording at %.0f FPS, scale %.2f, saving to %s"), 
        RecordingFPS, ResolutionScale, *RecordingDirectory);
    
    // 显示屏幕消息
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, 
            FString::Printf(TEXT("Viewport Recording Started (%.0f FPS)"), RecordingFPS));
    }
}

void UMAViewportRecorder::StopRecording()
{
    if (!bIsRecording)
    {
        return;
    }
    
    // 停止定时器
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RecordTimerHandle);
    }
    
    bIsRecording = false;
    
    UE_LOG(LogMAViewportRecorder, Log, TEXT("Stopped viewport recording. %d frames saved to %s"), 
        RecordingFrameIndex, *RecordingDirectory);
    
    // 显示屏幕消息
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
            FString::Printf(TEXT("Viewport Recording Stopped: %d frames saved"), RecordingFrameIndex));
    }
    
    // 自动编码为 MP4
    if (bAutoEncodeToMP4 && RecordingFrameIndex > 0)
    {
        EncodeToMP4();
    }
}

void UMAViewportRecorder::ToggleRecording()
{
    if (bIsRecording)
    {
        StopRecording();
    }
    else
    {
        StartRecording(RecordingFPS, ResolutionScale);
    }
}

void UMAViewportRecorder::OnRecordTick()
{
    CaptureFrame();
}

void UMAViewportRecorder::CaptureFrame()
{
    if (!bIsRecording) return;
    
    UWorld* World = GetWorld();
    if (!World) return;
    
    UGameViewportClient* ViewportClient = World->GetGameViewport();
    if (!ViewportClient) return;
    
    FViewport* Viewport = ViewportClient->Viewport;
    if (!Viewport) return;
    
    // 获取 Viewport 尺寸
    FIntPoint ViewportSize = Viewport->GetSizeXY();
    if (ViewportSize.X <= 0 || ViewportSize.Y <= 0) return;
    
    // 计算目标尺寸
    int32 TargetWidth = FMath::RoundToInt(ViewportSize.X * ResolutionScale);
    int32 TargetHeight = FMath::RoundToInt(ViewportSize.Y * ResolutionScale);
    
    // 读取 Viewport 像素
    TArray<FColor> Pixels;
    if (!Viewport->ReadPixels(Pixels))
    {
        UE_LOG(LogMAViewportRecorder, Warning, TEXT("Failed to read viewport pixels"));
        return;
    }
    
    // 如果需要缩放，进行简单的最近邻缩放
    TArray<FColor> FinalPixels;
    int32 FinalWidth, FinalHeight;
    
    if (FMath::Abs(ResolutionScale - 1.0f) < 0.01f)
    {
        // 不需要缩放
        FinalPixels = MoveTemp(Pixels);
        FinalWidth = ViewportSize.X;
        FinalHeight = ViewportSize.Y;
    }
    else
    {
        // 需要缩放
        FinalWidth = TargetWidth;
        FinalHeight = TargetHeight;
        FinalPixels.SetNum(FinalWidth * FinalHeight);
        
        for (int32 Y = 0; Y < FinalHeight; Y++)
        {
            for (int32 X = 0; X < FinalWidth; X++)
            {
                int32 SrcX = FMath::Clamp(FMath::RoundToInt(X / ResolutionScale), 0, ViewportSize.X - 1);
                int32 SrcY = FMath::Clamp(FMath::RoundToInt(Y / ResolutionScale), 0, ViewportSize.Y - 1);
                FinalPixels[Y * FinalWidth + X] = Pixels[SrcY * ViewportSize.X + SrcX];
            }
        }
    }
    
    // 压缩为 JPEG
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
    
    if (ImageWrapper.IsValid())
    {
        // FColor 是 BGRA 格式
        ImageWrapper->SetRaw(FinalPixels.GetData(), FinalPixels.Num() * sizeof(FColor), 
            FinalWidth, FinalHeight, ERGBFormat::BGRA, 8);
        
        TArray64<uint8> CompressedData = ImageWrapper->GetCompressed(JPEGQuality);
        
        if (CompressedData.Num() > 0)
        {
            // 保存文件
            FString FramePath = RecordingDirectory / FString::Printf(TEXT("frame_%06d.jpg"), RecordingFrameIndex);
            
            TArray<uint8> SaveData;
            SaveData.Append(CompressedData.GetData(), CompressedData.Num());
            
            if (FFileHelper::SaveArrayToFile(SaveData, *FramePath))
            {
                RecordingFrameIndex++;
            }
            else
            {
                UE_LOG(LogMAViewportRecorder, Warning, TEXT("Failed to save frame %d"), RecordingFrameIndex);
            }
        }
    }
}

void UMAViewportRecorder::OnViewportRendered(FViewport* Viewport, uint32 Unused)
{
    // 保留此方法以备将来使用回调方式捕获
}

void UMAViewportRecorder::EncodeToMP4()
{
    if (RecordingDirectory.IsEmpty() || RecordingFrameIndex == 0)
    {
        UE_LOG(LogMAViewportRecorder, Warning, TEXT("No frames to encode"));
        return;
    }
    
    // 构建输出文件路径
    FString OutputPath = RecordingDirectory / TEXT("output.mp4");
    FString InputPattern = RecordingDirectory / TEXT("frame_%06d.jpg");
    
    // 构建 FFmpeg 命令
    // -y: 覆盖输出文件
    // -framerate: 输入帧率
    // -i: 输入文件模式
    // -c:v libx264: 使用 H.264 编码
    // -pix_fmt yuv420p: 兼容性最好的像素格式
    // -crf 18: 质量参数 (0-51, 越小质量越高, 18 是视觉无损)
    FString FFmpegCmd = FString::Printf(
        TEXT("%s -y -framerate %.0f -i \"%s\" -c:v libx264 -pix_fmt yuv420p -crf 18 \"%s\""),
        *FFmpegPath, RecordingFPS, *InputPattern, *OutputPath
    );
    
    UE_LOG(LogMAViewportRecorder, Log, TEXT("Encoding to MP4: %s"), *FFmpegCmd);
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("Encoding video..."));
    }
    
    // 异步执行 FFmpeg
    FPlatformProcess::CreateProc(
        *FFmpegPath,
        *FString::Printf(TEXT("-y -framerate %.0f -i \"%s\" -c:v libx264 -pix_fmt yuv420p -crf 18 \"%s\""),
            RecordingFPS, *InputPattern, *OutputPath),
        true,   // bLaunchDetached
        false,  // bLaunchHidden
        false,  // bLaunchReallyHidden
        nullptr, // OutProcessID
        0,      // PriorityModifier
        nullptr, // OptionalWorkingDirectory
        nullptr  // PipeWriteChild
    );
    
    UE_LOG(LogMAViewportRecorder, Log, TEXT("FFmpeg encoding started. Output: %s"), *OutputPath);
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, 
            FString::Printf(TEXT("Video encoding started: %s"), *OutputPath));
    }
}
