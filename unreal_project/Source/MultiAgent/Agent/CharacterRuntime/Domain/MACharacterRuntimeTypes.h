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
