#pragma once

#include "CoreMinimal.h"

class UMACameraSensorComponent;
class UTimerManager;
struct FTimerHandle;

struct MULTIAGENT_API FMASensingRuntimeBridge
{
    static bool StartStreamTimer(UMACameraSensorComponent& CameraSensor, FTimerHandle& TimerHandle, float IntervalSeconds);
    static void ClearStreamTimer(UMACameraSensorComponent& CameraSensor, FTimerHandle& TimerHandle);
};
