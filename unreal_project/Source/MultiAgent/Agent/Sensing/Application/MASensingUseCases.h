#pragma once

#include "CoreMinimal.h"
#include "Agent/Sensing/Feedback/MASensingFeedback.h"

struct MULTIAGENT_API FMASensingUseCases
{
    static FMASensingOperationFeedback BuildPhotoSaveTarget(const FString& SensorName, const FString& RequestedPath);
};
