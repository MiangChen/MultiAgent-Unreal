#pragma once

#include "CoreMinimal.h"

struct FMAConfigSnapshot;

namespace MAConfigLoadUseCases
{
    MULTIAGENT_API bool LoadAllConfigs(FMAConfigSnapshot& Snapshot);
    MULTIAGENT_API bool ReloadConfigs(FMAConfigSnapshot& Snapshot);
}
