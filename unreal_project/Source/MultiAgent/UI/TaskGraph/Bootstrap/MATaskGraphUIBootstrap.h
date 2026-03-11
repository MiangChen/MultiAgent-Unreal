#pragma once

#include "CoreMinimal.h"

class UMATaskPlannerWidget;
class UMANodePaletteWidget;
class UMAUITheme;

class MULTIAGENT_API FMATaskGraphUIBootstrap
{
public:
    void ConfigureTaskPlannerWidget(UMATaskPlannerWidget* Widget, UMAUITheme* Theme) const;
    void InitializeNodePalette(UMANodePaletteWidget* NodePalette) const;
};
