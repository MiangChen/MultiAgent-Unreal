#include "Agent/Sensing/Infrastructure/MASensingRuntimeBridge.h"

#include "Agent/Sensing/Runtime/MACameraSensorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

bool FMASensingRuntimeBridge::StartStreamTimer(
    UMACameraSensorComponent& CameraSensor,
    FTimerHandle& TimerHandle,
    const float IntervalSeconds)
{
    UWorld* World = CameraSensor.GetWorld();
    if (!World)
    {
        return false;
    }

    FTimerManager& TimerManager = World->GetTimerManager();
    if (TimerHandle.IsValid())
    {
        TimerManager.ClearTimer(TimerHandle);
    }

    TimerManager.SetTimer(TimerHandle, &CameraSensor, &UMACameraSensorComponent::OnStreamTick, IntervalSeconds, true);
    return true;
}

void FMASensingRuntimeBridge::ClearStreamTimer(UMACameraSensorComponent& CameraSensor, FTimerHandle& TimerHandle)
{
    if (UWorld* World = CameraSensor.GetWorld())
    {
        World->GetTimerManager().ClearTimer(TimerHandle);
    }
}
