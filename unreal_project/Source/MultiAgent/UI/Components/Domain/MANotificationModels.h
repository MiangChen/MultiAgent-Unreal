#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "../Core/MAHUDTypes.h"

class UMAUITheme;

struct FMANotificationContentModel
{
    FString Message;
    FString KeyHint;
    FLinearColor BackgroundColor = FLinearColor(0.1f, 0.1f, 0.12f, 0.95f);
};

struct FMANotificationAnimationState
{
    EMANotificationType Type = EMANotificationType::None;
    bool bVisible = false;
    bool bAnimating = false;
    bool bShowing = false;
    float CurrentAnimTime = 0.0f;
    float CurrentOffsetX = -300.0f;
    float CurrentOpacity = 0.0f;
};

struct FMANotificationAnimationFrame
{
    float OffsetX = -300.0f;
    float Opacity = 0.0f;
    bool bFinished = false;
};
