#pragma once

#include "CoreMinimal.h"

class AMAPlayerController;

class MULTIAGENT_API FMASquadInputCoordinator
{
public:
    void HandleControlGroup(AMAPlayerController* PlayerController, int32 GroupIndex) const;
    void CycleFormation(AMAPlayerController* PlayerController) const;
    void CreateSquad(AMAPlayerController* PlayerController) const;
    void DisbandSquad(AMAPlayerController* PlayerController) const;
};
