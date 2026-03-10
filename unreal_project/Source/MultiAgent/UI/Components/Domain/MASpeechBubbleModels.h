#pragma once

#include "CoreMinimal.h"

struct FMASpeechBubbleState
{
    bool bVisible = false;
    bool bFadingIn = false;
    bool bFadingOut = false;
    float AutoHideDuration = 10.0f;
    float ElapsedTime = 0.0f;
    float FadeProgress = 0.0f;
};

struct FMASpeechBubbleAnimationFrame
{
    float Opacity = 0.0f;
    bool bHideNow = false;
};
