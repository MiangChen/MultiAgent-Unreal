#pragma once

#include "CoreMinimal.h"
#include "../Feedback/MAFeedback21.h"

class AMAPlayerController;

class MULTIAGENT_API FMASquadInputCoordinator
{
public:
    void HandleControlGroup(AMAPlayerController* PlayerController, int32 GroupIndex) const;
    FMAFeedback21Batch CycleFormation(AMAPlayerController* PlayerController) const;
    FMAFeedback21Batch CreateSquad(AMAPlayerController* PlayerController) const;
    FMAFeedback21Batch DisbandSquad(AMAPlayerController* PlayerController) const;
};
