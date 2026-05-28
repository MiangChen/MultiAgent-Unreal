#pragma once

#include "CoreMinimal.h"
#include "Core/Command/Domain/MACommandTypes.h"
#include "../Feedback/MAFeedback21.h"
#include "../Feedback/MAFeedbackPipeline.h"

class AMAPlayerController;

class MULTIAGENT_API FMACommandInputCoordinator
{
public:
    FMAFeedback21Batch SendCommandToSelection(AMAPlayerController* PlayerController, EMACommand Command) const;
    FMAFeedback21Batch TogglePauseExecution(AMAPlayerController* PlayerController) const;

private:
    FMAFeedbackPipeline FeedbackPipeline;
};
