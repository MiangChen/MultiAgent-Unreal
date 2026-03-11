#pragma once

#include "CoreMinimal.h"

class UMAStateTreeComponent;

struct MULTIAGENT_API FMAStateTreeRuntimeBridge
{
    static void RestartLogicNextTick(UMAStateTreeComponent& StateTreeComponent);
};
