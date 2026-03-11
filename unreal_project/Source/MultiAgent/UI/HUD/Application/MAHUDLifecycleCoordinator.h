// MAHUDLifecycleCoordinator.h
// HUD lifecycle coordination.

#pragma once

#include "CoreMinimal.h"

class AMAHUD;

class MULTIAGENT_API FMAHUDLifecycleCoordinator
{
public:
    void BindRuntimeDelegates(AMAHUD* HUD) const;
};
