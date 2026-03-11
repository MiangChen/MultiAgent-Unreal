#pragma once

#include "CoreMinimal.h"

class UMADirectControlIndicator;
class UMASystemLogPanel;
class UMAPreviewPanel;
class UMAInstructionPanel;
class UMAUITheme;

class MULTIAGENT_API FMAComponentsBootstrap
{
public:
    void ConfigureDirectControlIndicator(UMADirectControlIndicator* Widget, UMAUITheme* Theme) const;
    void ConfigureSystemLogPanel(UMASystemLogPanel* Widget, UMAUITheme* Theme) const;
    void ConfigurePreviewPanel(UMAPreviewPanel* Widget, UMAUITheme* Theme) const;
    void ConfigureInstructionPanel(UMAInstructionPanel* Widget, UMAUITheme* Theme) const;
};
