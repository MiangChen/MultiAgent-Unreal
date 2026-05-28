#pragma once

#include "CoreMinimal.h"
#include "Agent/StateTree/Domain/MAStateTreeTypes.h"

class AMAChargingStation;

struct MULTIAGENT_API FMAStateTreeLifecycleFeedback
{
    bool bShouldStartLogic = false;
    EMAStateTreeLifecycleMode Mode = EMAStateTreeLifecycleMode::Disabled;
    FString Message;
};

struct MULTIAGENT_API FMAStateTreeTaskExitFeedback
{
    bool bShouldCancelCommand = false;
    bool bShouldTransitionCommandToIdle = false;
};

struct MULTIAGENT_API FMAStateTreeChargeExitFeedback
{
    bool bShouldCancelNavigate = false;
    bool bShouldCancelCharge = false;
};

struct MULTIAGENT_API FMAStateTreeChargeStationFeedback
{
    bool bFoundStation = false;
    FVector StationLocation = FVector::ZeroVector;
    TWeakObjectPtr<AMAChargingStation> ChargingStation;
    float InteractionRadius = 150.f;
};
