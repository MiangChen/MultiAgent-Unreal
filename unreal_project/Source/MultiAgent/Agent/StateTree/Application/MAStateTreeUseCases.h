#pragma once

#include "CoreMinimal.h"
#include "Agent/StateTree/Feedback/MAStateTreeFeedback.h"

struct MULTIAGENT_API FMAStateTreeUseCases
{
    static FMAStateTreeLifecycleFeedback BuildBeginPlayFeedback(bool bStateTreeEnabled, bool bHasAsset);
    static FMAStateTreeLifecycleFeedback BuildEnableFeedback(bool bHasAsset);
};
