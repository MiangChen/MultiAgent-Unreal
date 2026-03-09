#pragma once

#include "CoreMinimal.h"

class AMAPlayerController;

class MULTIAGENT_API FMAInteractionBootstrap
{
public:
    void InitializePlayerController(AMAPlayerController* PlayerController) const;
};
