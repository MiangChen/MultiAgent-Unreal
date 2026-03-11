// MACameraSensorComponent.cpp

#include "MACameraSensorComponent.h"
#include "Agent/Sensing/Application/MASensingUseCases.h"
#include "Agent/Sensing/Bootstrap/MASensingBootstrap.h"
#include "Agent/Sensing/Infrastructure/MASensingRuntimeBridge.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "TimerManager.h"
#include "RenderingThread.h"

UMACameraSensorComponent::UMACameraSensorComponent()
{
    SensorType = EMASensorType::Camera;
    SensorName = TEXT("Camera");
    
    // 子组件将在 OnRegister 中创建，因为 CreateDefaultSubobject 只能在 CDO 构造时使用
    // 运行时通过 NewObject 创建的组件需要在 OnRegister 中动态创建子组件
}

void UMACameraSensorComponent::OnRegister()
{
    Super::OnRegister();
    
    // 动态创建子组件（支持运行时 NewObject 创建）
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

bool UMACameraSensorComponent::TakePhoto(const FString& FilePath)
{
    const FMASensingOperationFeedback PathFeedback =
        FMASensingUseCases::BuildPhotoSaveTarget(SensorName, FilePath);
    FString SavePath = PathFeedback.ResolvedPath;
    
    FString Directory = FPaths::GetPath(SavePath);
    IFileManager::Get().MakeDirectory(*Directory, true);
    
    if (RenderTarget)
    {
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

TArray<FColor> UMACameraSensorComponent::CaptureFrame()
{
    TArray<FColor> Pixels;
    
    if (!RenderTarget || !SceneCaptureComponent || !SceneCaptureComponent->TextureTarget)
    {
        return Pixels;
    }
    
    SceneCaptureComponent->CaptureScene();
    FlushRenderingCommands();
    
    FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
    if (RenderTargetResource)
    {
        FReadSurfaceDataFlags ReadSurfaceDataFlags;
        ReadSurfaceDataFlags.SetLinearToGamma(false);
        RenderTargetResource->ReadPixels(Pixels, ReadSurfaceDataFlags);
    }
    
    return Pixels;
}

// ========== 通用帧获取 ==========
TArray<uint8> UMACameraSensorComponent::GetFrameAsJPEG(int32 Quality)
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

// ========== TCP 流 ==========
bool UMACameraSensorComponent::StartTCPStream(int32 Port, float FPS)
{
    const FMASensingStreamFeedback Feedback =
        FMASensingUseCases::BuildStartStreamFeedback(bIsStreaming, FPS);
    if (!Feedback.bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Camera] %s: %s"), *SensorName, *Feedback.Message);
        return false;
    }

    StreamPort = Port;
    
    UE_LOG(LogTemp, Log, TEXT("[Camera] %s attempting to create TCP listener on port %d..."), *SensorName, Port);

    if (!FMASensingRuntimeBridge::CreateListenSocket(*this, Port))
    {
        UE_LOG(LogTemp, Error, TEXT("[Camera] %s failed to create/bind/listen on port %d"), *SensorName, Port);
        return false;
    }

    bIsStreaming = true;

    if (!FMASensingRuntimeBridge::StartStreamTimer(*this, StreamTimerHandle, Feedback.IntervalSeconds))
    {
        bIsStreaming = false;
        FMASensingRuntimeBridge::CleanupSockets(*this);
        UE_LOG(LogTemp, Error, TEXT("[Camera] %s failed to start stream timer"), *SensorName);
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[Camera] %s started TCP stream on port %d at %.0f FPS"), *SensorName, Port, FPS);
    return true;
}

void UMACameraSensorComponent::StopTCPStream()
{
    const FMASensingStreamFeedback Feedback =
        FMASensingUseCases::BuildStopStreamFeedback(bIsStreaming);
    if (!Feedback.bShouldCleanup)
    {
        return;
    }

    FMASensingRuntimeBridge::ClearStreamTimer(*this, StreamTimerHandle);
    FMASensingRuntimeBridge::CleanupSockets(*this);
    bIsStreaming = false;
    
    UE_LOG(LogTemp, Log, TEXT("[Camera] %s stopped TCP stream"), *SensorName);
}

void UMACameraSensorComponent::OnStreamTick()
{
    AcceptNewClients();
    
    if (ClientSockets.Num() == 0) return;
    
    TArray<uint8> JPEGData = GetFrameAsJPEG(JPEGQuality);
    if (JPEGData.Num() > 0)
    {
        SendFrameToClients(JPEGData);
    }
}

void UMACameraSensorComponent::AcceptNewClients()
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

void UMACameraSensorComponent::SendFrameToClients(const TArray<uint8>& JPEGData)
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

// ========== Action 动态发现与执行 ==========

TArray<FString> UMACameraSensorComponent::GetAvailableActions() const
{
    TArray<FString> Actions;
    
    // 拍照
    Actions.Add(TEXT("Camera.TakePhoto"));
    
    // TCP 流
    Actions.Add(TEXT("Camera.StartTCPStream"));
    Actions.Add(TEXT("Camera.StopTCPStream"));
    
    // 获取帧
    Actions.Add(TEXT("Camera.GetFrameAsJPEG"));
    
    return Actions;
}

bool UMACameraSensorComponent::ExecuteAction(const FString& ActionName, const TMap<FString, FString>& Params)
{
    const FMASensingActionRequest Request =
        FMASensingUseCases::BuildActionRequest(ActionName, Params);
    if (!Request.bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Camera] Unknown action: %s"), *ActionName);
        return false;
    }

    if (Request.Action == EMASensingActionKind::TakePhoto)
    {
        return TakePhoto(Request.FilePath);
    }

    if (Request.Action == EMASensingActionKind::StartTCPStream)
    {
        return StartTCPStream(Request.Port, Request.FPS);
    }

    if (Request.Action == EMASensingActionKind::StopTCPStream)
    {
        StopTCPStream();
        return true;
    }

    if (Request.Action == EMASensingActionKind::GetFrameAsJPEG)
    {
        TArray<uint8> Data = GetFrameAsJPEG(Request.Quality);
        return Data.Num() > 0;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Camera] Unsupported action: %s"), *ActionName);
    return false;
}
