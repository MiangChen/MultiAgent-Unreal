#pragma once

#include "CoreMinimal.h"

class UMAModifyWidget;
class UMAEditWidget;
class UMASceneListWidget;
class UMAUITheme;

class MULTIAGENT_API FMASceneEditingBootstrap
{
public:
    void ConfigureModifyWidget(UMAModifyWidget* Widget, UMAUITheme* Theme) const;
    void ConfigureEditWidget(UMAEditWidget* Widget, UMAUITheme* Theme) const;
    void ConfigureSceneListWidget(UMASceneListWidget* Widget, UMAUITheme* Theme) const;
};
