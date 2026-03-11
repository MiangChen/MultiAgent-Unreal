#pragma once

#include "CoreMinimal.h"

class AMAHUD;
class UMAUIManager;

class MULTIAGENT_API FMAUIBootstrap
{
public:
    UMAUIManager* CreateAndInitializeUIManager(AMAHUD* HUD) const;
    void ConfigureContextWidgets(UMAUIManager* UIManager) const;
};
