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

struct MULTIAGENT_API FMACharacterDirectControlFeedback
{
    EMACharacterDirectControlTransition Transition = EMACharacterDirectControlTransition::None;
    bool bShouldCancelAIMovement = false;
    bool bOrientRotationToMovement = true;
};

struct MULTIAGENT_API FMACharacterLowEnergyTriggerFeedback
{
    EMACharacterLowEnergyAction Action = EMACharacterLowEnergyAction::None;
    bool bShouldCancelSkills = false;
    bool bShouldBindPauseStateChanged = false;
    bool bShouldUnbindPauseStateChanged = false;
    bool bShouldClearPendingReturn = false;
    FString StatusMessage;
    float StatusDuration = 0.f;
    FString LogMessage;
};
