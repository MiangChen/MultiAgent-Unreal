#pragma once

#include "CoreMinimal.h"

class UMAHUDWidget;

class MULTIAGENT_API FMAHUDWidgetRuntimeAdapter
{
public:
    void ClearNotificationTimers(UMAHUDWidget* Widget) const;
    void ScheduleNotificationFadeOut(UMAHUDWidget* Widget, float DelaySeconds) const;
    void ScheduleFadeTick(UMAHUDWidget* Widget, float IntervalSeconds) const;

private:
    class UWorld* ResolveWorld(const UMAHUDWidget* Widget) const;
};
