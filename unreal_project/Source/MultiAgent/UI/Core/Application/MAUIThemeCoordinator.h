// MAUIThemeCoordinator.h
// UIManager Theme 协调器（L1 Application）

#pragma once

#include "CoreMinimal.h"

class UMAUIManager;
class UMAUITheme;

class MULTIAGENT_API FMAUIThemeCoordinator
{
public:
    void LoadTheme(UMAUIManager* UIManager, UMAUITheme* ThemeAsset) const;
    bool ValidateTheme(UMAUITheme* InTheme) const;
    void DistributeThemeToWidgets(UMAUIManager* UIManager) const;
};
