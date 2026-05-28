#pragma once

#include "CoreMinimal.h"

class UMASkillAllocationViewer;
class UMAUITheme;

class MULTIAGENT_API FMASkillAllocationUIBootstrap
{
public:
    void ConfigureSkillAllocationViewer(UMASkillAllocationViewer* Viewer, UMAUITheme* Theme) const;
};
