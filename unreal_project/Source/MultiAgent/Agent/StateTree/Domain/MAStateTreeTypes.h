#pragma once

#include "CoreMinimal.h"

enum class EMAStateTreeLifecycleMode : uint8
{
    Disabled,
    WaitingForAsset,
    Ready,
};
