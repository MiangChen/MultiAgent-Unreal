#include "Agent/Sensing/Application/MASensingUseCases.h"

#include "Misc/DateTime.h"
#include "Misc/Paths.h"

FMASensingOperationFeedback FMASensingUseCases::BuildPhotoSaveTarget(const FString& SensorName, const FString& RequestedPath)
{
    FMASensingOperationFeedback Feedback;
    Feedback.bSuccess = true;

    if (!RequestedPath.IsEmpty())
    {
        Feedback.ResolvedPath = RequestedPath;
        Feedback.Message = TEXT("Using explicit photo path");
        return Feedback;
    }

    const FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
    Feedback.ResolvedPath = FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString::Printf(TEXT("%s_%s.png"), *SensorName, *Timestamp);
    Feedback.Message = TEXT("Using default photo path");
    return Feedback;
}

FMASensingActionRequest FMASensingUseCases::BuildActionRequest(
    const FString& ActionName,
    const TMap<FString, FString>& Params)
{
    FMASensingActionRequest Request;
    Request.bSuccess = true;

    if (ActionName == TEXT("Camera.TakePhoto"))
    {
        Request.Action = EMASensingActionKind::TakePhoto;
        if (const FString* PathParam = Params.Find(TEXT("path")))
        {
            Request.FilePath = *PathParam;
        }
        return Request;
    }

    if (ActionName == TEXT("Camera.StartTCPStream"))
    {
        Request.Action = EMASensingActionKind::StartTCPStream;
        if (const FString* PortParam = Params.Find(TEXT("port")))
        {
            Request.Port = FCString::Atoi(**PortParam);
        }
        if (const FString* FPSParam = Params.Find(TEXT("fps")))
        {
            Request.FPS = FCString::Atof(**FPSParam);
        }
        return Request;
    }

    if (ActionName == TEXT("Camera.StopTCPStream"))
    {
        Request.Action = EMASensingActionKind::StopTCPStream;
        return Request;
    }

    if (ActionName == TEXT("Camera.GetFrameAsJPEG"))
    {
        Request.Action = EMASensingActionKind::GetFrameAsJPEG;
        if (const FString* QualityParam = Params.Find(TEXT("quality")))
        {
            Request.Quality = FCString::Atoi(**QualityParam);
        }
        return Request;
    }

    Request.bSuccess = false;
    Request.Action = EMASensingActionKind::Invalid;
    Request.Message = TEXT("Unknown sensing action");
    return Request;
}
