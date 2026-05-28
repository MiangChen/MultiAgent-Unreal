#pragma once

#include "CoreMinimal.h"

enum class EMACharacterReturnMode : uint8
{
    NavigateHome,
    ReturnHome,
};

enum class EMACharacterRuntimeSeverity : uint8
{
    Info,
    Warning,
    Error,
};

enum class EMACharacterDirectControlTransition : uint8
{
    None,
    Enable,
    Disable,
};

enum class EMACharacterLowEnergyAction : uint8
{
    None,
    DeferUntilResume,
    ExecuteReturn,
};
