// MACameraSensor.cpp

#include "MACameraSensor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Common/TcpSocketBuilder.h"
#include "TimerManager.h"

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
    
    SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    SceneCaptureComponent->bAlwaysPersistRenderingState = true;
    SceneCaptureComponent->ShowFlags.SetPostProcessing(true);
}

void AMACameraSensor::BeginDestroy()
{
    StopRecording();
    StopTCPStream();
    Super::BeginDestroy();
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
    RenderTarget->InitAutoFormat(Resolution.X, Resolution.Y);
    RenderTarget->TargetGamma = GEngine->GetDisplayGamma();
    RenderTarget->UpdateResourceImmediate();
    
    SceneCaptureComponent->TextureTarget = RenderTarget;
    
    UE_LOG(LogTemp, Log, TEXT("[Camera] %s initialized %dx%d, FOV %.0f"),
        *SensorName, Resolution.X, Resolution.Y, FOV);
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
    
    if (!RenderTarget) return Pixels;
    
    SceneCaptureComponent->CaptureScene();
    
    FReadSurfaceDataFlags ReadSurfaceDataFlags;
    ReadSurfaceDataFlags.SetLinearToGamma(false);
    RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadSurfaceDataFlags);
    
    return Pixels;
}

// ========== 通用帧获取 ==========
TArray<uint8> AMACameraSensor::GetFrameAsJPEG(int32 Quality)
{
    TArray<uint8> JPEGData;
    
    TArray<FColor> Pixels = CaptureFrame();
    if (Pixels.Num() == 0) return JPEGData;
    
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
    
    if (ImageWrapper.IsValid())
    {
        // FColor 是 BGRA 格式
        ImageWrapper->SetRaw(Pixels.GetData(), Pixels.Num() * sizeof(FColor), Resolution.X, Resolution.Y, ERGBFormat::BGRA, 8);
        TArray64<uint8> CompressedData = ImageWrapper->GetCompressed(Quality);
        JPEGData.Append(CompressedData.GetData(), CompressedData.Num());
    }
    
    return JPEGData;
}

// ========== 录像功能 ==========
void AMACameraSensor::StartRecording(float FPS)
{
    if (bIsRecording) return;
    
    FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
    RecordingDirectory = FPaths::ProjectSavedDir() / TEXT("Recordings") / FString::Printf(TEXT("%s_%s"), *SensorName, *Timestamp);
    IFileManager::Get().MakeDirectory(*RecordingDirectory, true);
    
    RecordingFrameIndex = 0;
    bIsRecording = true;
    
    float Interval = 1.0f / FPS;
    GetWorld()->GetTimerManager().SetTimer(RecordTimerHandle, this, &AMACameraSensor::OnRecordTick, Interval, true);
    
    UE_LOG(LogTemp, Log, TEXT("[Camera] %s started recording at %.0f FPS to %s"), *SensorName, FPS, *RecordingDirectory);
}

void AMACameraSensor::StopRecording()
{
    if (!bIsRecording) return;
    
    GetWorld()->GetTimerManager().ClearTimer(RecordTimerHandle);
    bIsRecording = false;
    
    UE_LOG(LogTemp, Log, TEXT("[Camera] %s stopped recording. %d frames saved to %s"), *SensorName, RecordingFrameIndex, *RecordingDirectory);
}

void AMACameraSensor::OnRecordTick()
{
    TArray<uint8> JPEGData = GetFrameAsJPEG(JPEGQuality);
    if (JPEGData.Num() > 0)
    {
        FString FramePath = RecordingDirectory / FString::Printf(TEXT("frame_%06d.jpg"), RecordingFrameIndex);
        FFileHelper::SaveArrayToFile(JPEGData, *FramePath);
        RecordingFrameIndex++;
    }
}

// ========== TCP 流 ==========
bool AMACameraSensor::StartTCPStream(int32 Port, float FPS)
{
    if (bIsStreaming) return false;
    
    StreamPort = Port;
    
    // 创建监听 Socket
    ListenSocket = FTcpSocketBuilder(TEXT("CameraStreamListener"))
        .AsReusable()
        .BoundToPort(Port)
        .Listening(8)
        .Build();
    
    if (!ListenSocket)
    {
        UE_LOG(LogTemp, Error, TEXT("[Camera] %s failed to create TCP listener on port %d"), *SensorName, Port);
        return false;
    }
    
    ListenSocket->SetNonBlocking(true);
    bIsStreaming = true;
    
    float Interval = 1.0f / FPS;
    GetWorld()->GetTimerManager().SetTimer(StreamTimerHandle, this, &AMACameraSensor::OnStreamTick, Interval, true);
    
    UE_LOG(LogTemp, Log, TEXT("[Camera] %s started TCP stream on port %d at %.0f FPS"), *SensorName, Port, FPS);
    return true;
}

void AMACameraSensor::StopTCPStream()
{
    if (!bIsStreaming) return;
    
    GetWorld()->GetTimerManager().ClearTimer(StreamTimerHandle);
    CleanupSockets();
    bIsStreaming = false;
    
    UE_LOG(LogTemp, Log, TEXT("[Camera] %s stopped TCP stream"), *SensorName);
}

void AMACameraSensor::OnStreamTick()
{
    AcceptNewClients();
    
    if (ClientSockets.Num() == 0) return;
    
    TArray<uint8> JPEGData = GetFrameAsJPEG(JPEGQuality);
    if (JPEGData.Num() > 0)
    {
        SendFrameToClients(JPEGData);
    }
}

void AMACameraSensor::AcceptNewClients()
{
    if (!ListenSocket) return;
    
    bool bHasPendingConnection = false;
    if (ListenSocket->HasPendingConnection(bHasPendingConnection) && bHasPendingConnection)
    {
        FSocket* ClientSocket = ListenSocket->Accept(TEXT("CameraStreamClient"));
        if (ClientSocket)
        {
            ClientSocket->SetNonBlocking(true);
            ClientSockets.Add(ClientSocket);
            UE_LOG(LogTemp, Log, TEXT("[Camera] %s new client connected. Total: %d"), *SensorName, ClientSockets.Num());
        }
    }
}

void AMACameraSensor::SendFrameToClients(const TArray<uint8>& JPEGData)
{
    // 帧格式: [4字节长度][JPEG数据]
    int32 DataSize = JPEGData.Num();
    
    TArray<FSocket*> DisconnectedClients;
    
    for (FSocket* Client : ClientSockets)
    {
        if (!Client) continue;
        
        int32 BytesSent = 0;
        
        // 发送长度头
        if (!Client->Send((uint8*)&DataSize, sizeof(int32), BytesSent) || BytesSent != sizeof(int32))
        {
            DisconnectedClients.Add(Client);
            continue;
        }
        
        // 发送 JPEG 数据
        if (!Client->Send(JPEGData.GetData(), DataSize, BytesSent) || BytesSent != DataSize)
        {
            DisconnectedClients.Add(Client);
            continue;
        }
    }
    
    // 清理断开的客户端
    for (FSocket* Client : DisconnectedClients)
    {
        ClientSockets.Remove(Client);
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Client);
        UE_LOG(LogTemp, Log, TEXT("[Camera] %s client disconnected. Remaining: %d"), *SensorName, ClientSockets.Num());
    }
}

void AMACameraSensor::CleanupSockets()
{
    for (FSocket* Client : ClientSockets)
    {
        if (Client)
        {
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Client);
        }
    }
    ClientSockets.Empty();
    
    if (ListenSocket)
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
        ListenSocket = nullptr;
    }
}
