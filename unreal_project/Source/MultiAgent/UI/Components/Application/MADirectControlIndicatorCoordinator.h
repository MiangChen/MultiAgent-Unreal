#pragma once

#include "CoreMinimal.h"
#include "../Domain/MADirectControlIndicatorModel.h"

class UMAUITheme;

class FMADirectControlIndicatorCoordinator
{
public:
    FMADirectControlIndicatorModel BuildModel(const FString& AgentLabel, const UMAUITheme* Theme, const FLinearColor& DefaultColor) const;
};
