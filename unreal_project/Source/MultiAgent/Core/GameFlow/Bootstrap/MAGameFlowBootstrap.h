#pragma once

#include "CoreMinimal.h"

// GameFlow remains a bootstrap shell for map startup, setup flow, and engine-level wiring.
struct MULTIAGENT_API FMAGameFlowBootstrap
{
    static constexpr bool bOwnsRuntimeBootSequence = true;
};
