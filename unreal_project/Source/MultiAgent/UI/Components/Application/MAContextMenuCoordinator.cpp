#include "MAContextMenuCoordinator.h"

#include "../Infrastructure/MAContextMenuLayout.h"
#include "../Core/MAUITheme.h"

FMAContextMenuPlacementModel FMAContextMenuCoordinator::BuildPlacement(const UUserWidget* Widget, const FVector2D& ScreenPosition) const
{
    FMAContextMenuPlacementModel Model;
    const FMAContextMenuLayout Layout;
    Model.ViewportPosition = Layout.ToViewportPosition(Widget, ScreenPosition);
    Model.Visibility = ESlateVisibility::Visible;
    return Model;
}

bool FMAContextMenuCoordinator::ShouldCloseForClick(const FVector2D& LocalPosition, const FVector2D& Size) const
{
    const FMAContextMenuLayout Layout;
    return Layout.IsOutsideBounds(LocalPosition, Size);
}

FMAContextMenuThemeModel FMAContextMenuCoordinator::BuildThemeModel(const UMAUITheme* Theme) const
{
    FMAContextMenuThemeModel Model;
    if (!Theme)
    {
        return Model;
    }

    Model.MenuBackgroundColor = Theme->BackgroundColor;
    Model.ItemDefaultColor = Theme->SecondaryColor;
    Model.ItemHoverColor = Theme->HighlightColor;
    Model.ItemDisabledColor = FLinearColor(
        Theme->SecondaryColor.R * 0.5f,
        Theme->SecondaryColor.G * 0.5f,
        Theme->SecondaryColor.B * 0.5f,
        0.5f
    );
    Model.TextColor = Theme->TextColor;
    Model.DisabledTextColor = FLinearColor(
        Theme->SecondaryTextColor.R,
        Theme->SecondaryTextColor.G,
        Theme->SecondaryTextColor.B,
        0.5f
    );
    return Model;
}
