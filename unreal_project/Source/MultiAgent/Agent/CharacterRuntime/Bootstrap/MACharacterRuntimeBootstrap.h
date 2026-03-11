#pragma once

#include "CoreMinimal.h"

class AMAUAVCharacter;

struct MULTIAGENT_API FMACharacterRuntimeBootstrap
{
    static void ApplyUAVFlightConfig(AMAUAVCharacter& UAVCharacter);
};
