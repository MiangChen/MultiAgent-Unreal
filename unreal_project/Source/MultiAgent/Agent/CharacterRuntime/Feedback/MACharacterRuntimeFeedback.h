#pragma once

#include "CoreMinimal.h"
#include "Agent/CharacterRuntime/Domain/MACharacterRuntimeTypes.h"

struct MULTIAGENT_API FMACharacterRuntimeFeedback
{
    bool bSuccess = false;
    EMACharacterReturnMode ReturnMode = EMACharacterReturnMode::NavigateHome;
    EMACharacterRuntimeSeverity Severity = EMACharacterRuntimeSeverity::Info;
    FString Message;
};
