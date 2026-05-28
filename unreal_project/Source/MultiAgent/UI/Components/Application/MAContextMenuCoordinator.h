#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAContextMenuModels.h"

class UUserWidget;
class UMAUITheme;

class FMAContextMenuCoordinator
{
public:
    FMAContextMenuPlacementModel BuildPlacement(const UUserWidget* Widget, const FVector2D& ScreenPosition) const;
    bool ShouldCloseForClick(const FVector2D& LocalPosition, const FVector2D& Size) const;
    FMAContextMenuThemeModel BuildThemeModel(const UMAUITheme* Theme) const;
};
