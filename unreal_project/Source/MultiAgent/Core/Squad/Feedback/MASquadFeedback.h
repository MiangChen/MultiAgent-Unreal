#pragma once

#include "CoreMinimal.h"
#include "Core/Shared/Types/MATypes.h"

enum class EMASquadOperationKind : uint8
{
    None,
    CycleFormation,
    CreateSquad,
    DisbandSquad,
};

enum class EMASquadFeedbackSeverity : uint8
{
    Info,
    Success,
    Warning,
    Error,
};

struct MULTIAGENT_API FMASquadOperationFeedback
{
    EMASquadOperationKind Kind = EMASquadOperationKind::None;
    EMASquadFeedbackSeverity Severity = EMASquadFeedbackSeverity::Info;
    bool bSuccess = false;
    FString Message;
    FString SquadName;
    int32 AffectedCount = 0;
    EMAFormationType FormationType = EMAFormationType::None;
};
