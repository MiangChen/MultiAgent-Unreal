#pragma once

#include "CoreMinimal.h"

class UMANavigationService;
class UWorld;

class MULTIAGENT_API FMANavigationBootstrap
{
public:
    static void ApplyConfig(UMANavigationService& NavigationService, UWorld* World);
};
