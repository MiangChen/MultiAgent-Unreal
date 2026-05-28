#pragma once

#include "CoreMinimal.h"
#include "../Domain/MASpeechBubbleModels.h"

class FMASpeechBubbleCoordinator
{
public:
    void BeginShow(FMASpeechBubbleState& State, float Duration) const;
    void BeginHide(FMASpeechBubbleState& State) const;
    FMASpeechBubbleAnimationFrame Step(FMASpeechBubbleState& State, float DeltaTime, float FadeInDuration, float FadeOutDuration) const;
};
