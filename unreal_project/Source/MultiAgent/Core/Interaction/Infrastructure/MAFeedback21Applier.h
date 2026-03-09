#pragma once

#include "CoreMinimal.h"
#include "../Feedback/MAFeedback21.h"
#include "MAHUDInputAdapter.h"

class APlayerController;
class AMAHUD;

class MULTIAGENT_API FMAFeedback21Applier
{
public:
    void ApplyToPlayerController(APlayerController* PlayerController, const FMAFeedback21Batch& Feedback) const;
    void ApplyToHUD(AMAHUD* HUD, const FMAFeedback21Batch& Feedback) const;

private:
    void ApplyHUDActionToPlayerController(APlayerController* PlayerController, EMAFeedback21HUDAction Action) const;
    void ApplyHUDActionToHUD(AMAHUD* HUD, EMAFeedback21HUDAction Action) const;
    void ShowPlayerMessage(const FMAFeedback21Message& Message) const;

    FMAHUDInputAdapter HUDInputAdapter;
};
