// L4 runtime bridge for HUD widget timers.

#include "MAHUDWidgetRuntimeAdapter.h"

#include "../Presentation/MAHUDWidget.h"
#include "Engine/World.h"
#include "TimerManager.h"

UWorld* FMAHUDWidgetRuntimeAdapter::ResolveWorld(const UMAHUDWidget* Widget) const
{
    return Widget ? Widget->GetWorld() : nullptr;
}

void FMAHUDWidgetRuntimeAdapter::ClearNotificationTimers(UMAHUDWidget* Widget) const
{
    if (UWorld* World = ResolveWorld(Widget))
    {
        World->GetTimerManager().ClearTimer(Widget->NotificationTimerHandle);
        World->GetTimerManager().ClearTimer(Widget->FadeTimerHandle);
    }
}

void FMAHUDWidgetRuntimeAdapter::ScheduleNotificationFadeOut(UMAHUDWidget* Widget, float DelaySeconds) const
{
    if (UWorld* World = ResolveWorld(Widget))
    {
        World->GetTimerManager().SetTimer(
            Widget->NotificationTimerHandle,
            Widget,
            &UMAHUDWidget::StartNotificationFadeOut,
            DelaySeconds,
            false
        );
    }
}

void FMAHUDWidgetRuntimeAdapter::ScheduleFadeTick(UMAHUDWidget* Widget, float IntervalSeconds) const
{
    if (UWorld* World = ResolveWorld(Widget))
    {
        World->GetTimerManager().SetTimer(
            Widget->FadeTimerHandle,
            Widget,
            &UMAHUDWidget::OnFadeTick,
            IntervalSeconds,
            true
        );
    }
}
