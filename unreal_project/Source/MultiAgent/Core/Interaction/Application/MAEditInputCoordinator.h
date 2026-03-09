#pragma once

#include "CoreMinimal.h"
#include "../Feedback/MAFeedback21.h"
#include "../Infrastructure/MAInteractionRuntimeAdapter.h"

class AMAPlayerController;

class MULTIAGENT_API FMAEditInputCoordinator
{
public:
    FMAFeedback21Batch HandleLeftClick(AMAPlayerController* PlayerController) const;
    FMAFeedback21Batch EnterMode(AMAPlayerController* PlayerController) const;
    FMAFeedback21Batch ExitMode(AMAPlayerController* PlayerController) const;

private:
    FMAInteractionRuntimeAdapter RuntimeAdapter;
};
