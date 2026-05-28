#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"

struct FMAContextMenuPlacementModel
{
    FVector2D ViewportPosition = FVector2D::ZeroVector;
    ESlateVisibility Visibility = ESlateVisibility::Collapsed;
};

struct FMAContextMenuThemeModel
{
    FLinearColor MenuBackgroundColor = FLinearColor(0.15f, 0.15f, 0.2f, 0.95f);
    FLinearColor ItemDefaultColor = FLinearColor(0.2f, 0.2f, 0.25f, 1.0f);
    FLinearColor ItemHoverColor = FLinearColor(0.3f, 0.3f, 0.4f, 1.0f);
    FLinearColor ItemDisabledColor = FLinearColor(0.15f, 0.15f, 0.15f, 0.5f);
    FLinearColor TextColor = FLinearColor(0.9f, 0.9f, 0.95f, 1.0f);
    FLinearColor DisabledTextColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);
};
