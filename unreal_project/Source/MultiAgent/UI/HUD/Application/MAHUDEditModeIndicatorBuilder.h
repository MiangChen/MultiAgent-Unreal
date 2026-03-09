#pragma once

#include "CoreMinimal.h"

class UWorld;

struct FMAHUDEditModeIndicatorModel
{
    TArray<FString> POIInfos;
    TArray<FString> GoalInfos;
    TArray<FString> ZoneInfos;
};

class MULTIAGENT_API FMAHUDEditModeIndicatorBuilder
{
public:
    bool Build(UWorld* World, FMAHUDEditModeIndicatorModel& OutModel) const;
};
