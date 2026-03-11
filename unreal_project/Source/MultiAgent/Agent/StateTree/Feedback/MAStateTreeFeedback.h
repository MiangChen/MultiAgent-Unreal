#pragma once

#include "CoreMinimal.h"
#include "Agent/StateTree/Domain/MAStateTreeTypes.h"

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
