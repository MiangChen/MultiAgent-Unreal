#include "MACameraSensorComponent.h"

#include "Agent/Sensing/Application/MASensingUseCases.h"

TArray<FString> UMACameraSensorComponent::GetAvailableActions() const
{
    return FMASensingUseCases::BuildAvailableCameraActions();
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

    switch (Request.Action)
    {
        case EMASensingActionKind::TakePhoto:
            return TakePhoto(Request.FilePath);
        case EMASensingActionKind::StartTCPStream:
            return StartTCPStream(Request.Port, Request.FPS);
        case EMASensingActionKind::StopTCPStream:
            StopTCPStream();
            return true;
        case EMASensingActionKind::GetFrameAsJPEG:
            return GetFrameAsJPEG(Request.Quality).Num() > 0;
        default:
            UE_LOG(LogTemp, Warning, TEXT("[Camera] Unsupported action: %s"), *ActionName);
            return false;
    }
}
