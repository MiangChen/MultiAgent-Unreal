#pragma once

#include "CoreMinimal.h"

class UObject;
class UMASquadManager;

struct MULTIAGENT_API FMASquadBootstrap
{
    static UMASquadManager* Resolve(const UObject* WorldContext);
};
