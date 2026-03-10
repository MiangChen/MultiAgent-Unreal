#pragma once

#include "CoreMinimal.h"

enum class EMAHUDWidgetNotificationSeverity : uint8
{
    Info,
    Warning,
    Error,
};

struct FMAHUDWidgetEditModeViewModel
{
    bool bVisible = false;
    FString POIText;
    FString GoalText;
    FString ZoneText;
};

struct FMAHUDWidgetNotificationModel
{
    FString Message;
    EMAHUDWidgetNotificationSeverity Severity = EMAHUDWidgetNotificationSeverity::Info;
};
