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
