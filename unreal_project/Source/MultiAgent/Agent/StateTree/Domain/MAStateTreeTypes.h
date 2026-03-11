#pragma once

#include "CoreMinimal.h"

enum class EMAStateTreeLifecycleMode : uint8
{
    Disabled,
    WaitingForAsset,
    Ready,
};

enum class EMAStateTreeTaskDecision : uint8
{
    Failed,
    Running,
    Succeeded,
};
