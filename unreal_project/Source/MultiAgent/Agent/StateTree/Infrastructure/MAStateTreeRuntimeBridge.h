#pragma once

#include "CoreMinimal.h"
#include "Agent/StateTree/Feedback/MAStateTreeFeedback.h"

class UMAStateTreeComponent;
class AActor;

struct MULTIAGENT_API FMAStateTreeRuntimeBridge
{
    static void RestartLogicNextTick(UMAStateTreeComponent& StateTreeComponent);
    static FMAStateTreeChargeStationFeedback ResolveNearestChargingStation(const AActor* WorldContext, const FVector& FromLocation);
};
