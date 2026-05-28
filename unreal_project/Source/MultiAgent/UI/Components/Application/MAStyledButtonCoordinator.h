#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAStyledButtonModels.h"

class UMAUITheme;
enum class EMAButtonStyle : uint8;
struct FMAButtonAnimationState;

class FMAStyledButtonCoordinator
{
public:
    FMAStyledButtonPalette BuildPalette(EMAButtonStyle Style, const UMAUITheme* Theme, float DefaultCornerRadius) const;
    void StepAnimation(FMAButtonAnimationState& Current, const FMAButtonAnimationState& Target, float DeltaTime, float InterpSpeed) const;
};
