#pragma once

#include "CoreMinimal.h"

class UMAUITheme;
enum class EMAButtonStyle : uint8;

struct FMAStyledButtonPalette
{
    FLinearColor BaseColor = FLinearColor::White;
    FLinearColor HoverColor = FLinearColor::White;
    FLinearColor TextColor = FLinearColor::White;
    float CornerRadius = 8.0f;
};
