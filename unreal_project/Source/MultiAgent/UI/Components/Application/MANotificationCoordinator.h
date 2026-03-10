#pragma once

#include "CoreMinimal.h"
#include "../Domain/MANotificationModels.h"

class UMAUITheme;

class FMANotificationCoordinator
{
public:
    void BeginShow(FMANotificationAnimationState& State, EMANotificationType Type) const;
    void BeginHide(FMANotificationAnimationState& State) const;
    FMANotificationContentModel BuildContentModel(EMANotificationType Type, const UMAUITheme* Theme) const;
    FMANotificationAnimationFrame StepAnimation(FMANotificationAnimationState& State, float DeltaTime, float SlideInDuration, float SlideOutDuration, float StartOffsetX, float EndOffsetX) const;
    void FinishAnimation(FMANotificationAnimationState& State) const;
    FString GetNotificationMessage(EMANotificationType Type) const;
    FString GetKeyHint(EMANotificationType Type) const;
    FLinearColor GetBackgroundColorForType(EMANotificationType Type, const UMAUITheme* Theme) const;
};
