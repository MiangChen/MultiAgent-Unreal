#pragma once

#include "CoreMinimal.h"
#include "Core/Squad/Feedback/MASquadFeedback.h"

class AMACharacter;
class UObject;

struct MULTIAGENT_API FMASquadRuntimeBridge
{
    static FMASquadOperationFeedback CycleFormation(const UObject* WorldContext);
    static FMASquadOperationFeedback CreateSquadFromSelection(
        const UObject* WorldContext,
        const TArray<AMACharacter*>& SelectedAgents);
    static FMASquadOperationFeedback DisbandSelectedSquads(
        const UObject* WorldContext,
        const TArray<AMACharacter*>& SelectedAgents);
};
