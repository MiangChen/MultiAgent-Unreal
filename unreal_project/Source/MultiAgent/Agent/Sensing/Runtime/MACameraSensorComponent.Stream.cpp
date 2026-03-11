#include "MACameraSensorComponent.h"

#include "Agent/Sensing/Application/MASensingUseCases.h"
#include "Agent/Sensing/Infrastructure/MASensingRuntimeBridge.h"

bool UMACameraSensorComponent::StartTCPStream(const int32 Port, const float FPS)
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
    FMASensingRuntimeBridge::AcceptPendingClients(*this);
    if (ClientSockets.Num() == 0)
    {
        return;
    }

    const TArray<uint8> JPEGData = GetFrameAsJPEG(JPEGQuality);
    if (JPEGData.Num() > 0)
    {
        FMASensingRuntimeBridge::SendFrameToClients(*this, JPEGData);
    }
}
