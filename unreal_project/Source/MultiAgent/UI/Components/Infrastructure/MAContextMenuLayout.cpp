#include "MAContextMenuLayout.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"

FVector2D FMAContextMenuLayout::ToViewportPosition(const UUserWidget* Widget, const FVector2D& ScreenPosition) const
{
    float ViewportScale = 1.0f;
    if (Widget)
    {
        if (APlayerController* PC = Widget->GetOwningPlayer())
        {
            ViewportScale = UWidgetLayoutLibrary::GetViewportScale(PC);
        }
    }

    return ScreenPosition / ViewportScale;
}

bool FMAContextMenuLayout::IsOutsideBounds(const FVector2D& LocalPosition, const FVector2D& Size) const
{
    return LocalPosition.X < 0.0f || LocalPosition.Y < 0.0f || LocalPosition.X > Size.X || LocalPosition.Y > Size.Y;
}
