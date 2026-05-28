#pragma once

#include "CoreMinimal.h"

class UUserWidget;

class FMAContextMenuLayout
{
public:
    FVector2D ToViewportPosition(const UUserWidget* Widget, const FVector2D& ScreenPosition) const;
    bool IsOutsideBounds(const FVector2D& LocalPosition, const FVector2D& Size) const;
};
