#include "MASpeechBubbleCoordinator.h"

void FMASpeechBubbleCoordinator::BeginShow(FMASpeechBubbleState& State, float Duration) const
{
    State.AutoHideDuration = Duration;
    State.ElapsedTime = 0.0f;
    State.bVisible = true;
    State.bFadingIn = true;
    State.bFadingOut = false;
    State.FadeProgress = 0.0f;
}

void FMASpeechBubbleCoordinator::BeginHide(FMASpeechBubbleState& State) const
{
    if (!State.bVisible && !State.bFadingIn)
    {
        return;
    }

    State.bFadingOut = true;
    State.bFadingIn = false;
    State.FadeProgress = 0.0f;
}

FMASpeechBubbleAnimationFrame FMASpeechBubbleCoordinator::Step(FMASpeechBubbleState& State, float DeltaTime, float FadeInDuration, float FadeOutDuration) const
{
    FMASpeechBubbleAnimationFrame Frame;

    if (State.bFadingIn)
    {
        State.FadeProgress += DeltaTime / FadeInDuration;
        if (State.FadeProgress >= 1.0f)
        {
            State.FadeProgress = 1.0f;
            State.bFadingIn = false;
        }
        Frame.Opacity = 1.0f - FMath::Pow(1.0f - State.FadeProgress, 3.0f);
        return Frame;
    }

    if (State.bFadingOut)
    {
        State.FadeProgress += DeltaTime / FadeOutDuration;
        if (State.FadeProgress >= 1.0f)
        {
            State.FadeProgress = 1.0f;
            State.bFadingOut = false;
            State.bVisible = false;
            Frame.bHideNow = true;
            Frame.Opacity = 0.0f;
            return Frame;
        }
        Frame.Opacity = FMath::Pow(1.0f - State.FadeProgress, 2.0f);
        return Frame;
    }

    if (State.bVisible && State.AutoHideDuration > 0.0f)
    {
        State.ElapsedTime += DeltaTime;
        if (State.ElapsedTime >= State.AutoHideDuration)
        {
            BeginHide(State);
        }
    }

    Frame.Opacity = 1.0f;
    return Frame;
}
