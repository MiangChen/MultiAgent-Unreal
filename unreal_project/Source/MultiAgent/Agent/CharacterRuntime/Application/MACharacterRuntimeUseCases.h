#pragma once

#include "CoreMinimal.h"
#include "Agent/CharacterRuntime/Feedback/MACharacterRuntimeFeedback.h"

struct MULTIAGENT_API FMACharacterRuntimeUseCases
{
    static FMACharacterRuntimeFeedback BuildLowEnergyReturnPlan(bool bIsFlying);
};
