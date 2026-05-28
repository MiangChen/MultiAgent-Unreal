#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"

struct FMAModalButtonModel
{
    ESlateVisibility EditVisibility = ESlateVisibility::Collapsed;
};

struct FMAModalAnimationStep
{
    float NextTime = 0.0f;
    float Opacity = 1.0f;
    bool bFinished = false;
};
