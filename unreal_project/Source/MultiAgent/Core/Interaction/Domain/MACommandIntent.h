#pragma once

#include "CoreMinimal.h"

enum class EMAInteractionCommandType : uint8
{
    None,
    Charge,
    Idle,
    Follow,
    Navigate,
    CreateSquad,
    DisbandSquad
};

struct FMACommandIntent
{
    EMAInteractionCommandType Type = EMAInteractionCommandType::None;
    FString TargetId;
    FVector TargetLocation = FVector::ZeroVector;
};
