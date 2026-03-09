#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAHUDEditModeIndicatorModel.h"
#include "../Infrastructure/MAHUDEditRuntimeAdapter.h"

class AMAHUD;

class MULTIAGENT_API FMAHUDEditModeIndicatorBuilder
{
public:
    bool Build(AMAHUD* HUD, FMAHUDEditModeIndicatorModel& OutModel) const;

private:
    FMAHUDEditRuntimeAdapter RuntimeAdapter;
};
