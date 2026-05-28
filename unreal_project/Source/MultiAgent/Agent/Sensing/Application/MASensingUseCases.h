#pragma once

#include "CoreMinimal.h"
#include "Agent/Sensing/Feedback/MASensingFeedback.h"

struct MULTIAGENT_API FMASensingUseCases
{
    static TArray<FString> BuildAvailableCameraActions();
    static FMASensingOperationFeedback BuildPhotoSaveTarget(const FString& SensorName, const FString& RequestedPath);
    static FMASensingActionRequest BuildActionRequest(const FString& ActionName, const TMap<FString, FString>& Params);
    static FMASensingStreamFeedback BuildStartStreamFeedback(bool bIsStreaming, float FPS);
    static FMASensingStreamFeedback BuildStopStreamFeedback(bool bIsStreaming);
};
