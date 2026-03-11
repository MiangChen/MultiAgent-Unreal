#pragma once

#include "CoreMinimal.h"
#include "Agent/Navigation/Feedback/MANavigationFeedback.h"

class ACharacter;

struct MULTIAGENT_API FMANavigationUseCases
{
    static FMANavigationCommandFeedback BuildFlightCommandPrecheck(
        const ACharacter* OwnerCharacter,
        bool bIsFlying,
        bool bHasFlightController,
        const TCHAR* CommandName);
};
