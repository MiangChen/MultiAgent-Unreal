#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAHUDWidgetModels.h"

struct FMAHUDEditModeFeedback
{
    bool bVisible = false;
    TArray<FString> POIInfos;
    TArray<FString> GoalInfos;
    TArray<FString> ZoneInfos;
};

struct FMAHUDNotificationFeedback
{
    FString Message;
    bool bIsError = false;
    bool bIsWarning = false;
};
