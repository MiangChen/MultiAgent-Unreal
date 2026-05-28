#pragma once

#include "CoreMinimal.h"
#include "Core/Squad/Feedback/MASquadFeedback.h"

class UMASquad;

struct MULTIAGENT_API FMASquadUseCases
{
    static FMASquadOperationFeedback BuildRuntimeUnavailable(EMASquadOperationKind Kind);
    static FMASquadOperationFeedback BuildMissingDefaultSquad();
    static FMASquadOperationFeedback BuildSelectionTooSmall(int32 SelectedCount);
    static FMASquadOperationFeedback BuildCreateFailed();
    static FMASquadOperationFeedback BuildCreated(const UMASquad& Squad);
    static FMASquadOperationFeedback BuildDisbandEmpty();
    static FMASquadOperationFeedback BuildDisbanded(int32 DisbandedCount);
    static FMASquadOperationFeedback BuildCycleResult(const UMASquad& Squad);
};
