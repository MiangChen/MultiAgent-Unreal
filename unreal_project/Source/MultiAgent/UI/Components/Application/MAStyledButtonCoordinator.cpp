#include "MAStyledButtonCoordinator.h"

#include "../Core/MAUITheme.h"
#include "../Presentation/MAStyledButton.h"
#include "../Core/MARoundedBorderUtils.h"

FMAStyledButtonPalette FMAStyledButtonCoordinator::BuildPalette(EMAButtonStyle Style, const UMAUITheme* Theme, float DefaultCornerRadius) const
{
    FMAStyledButtonPalette Palette;
    Palette.CornerRadius = Theme ? MARoundedBorderUtils::GetCornerRadiusForType(Theme, EMARoundedElementType::Button) : DefaultCornerRadius;
    Palette.TextColor = Theme ? Theme->TextColor : FLinearColor(0.95f, 0.95f, 0.95f, 1.0f);

    auto ResolveBase = [&](EMAButtonStyle InStyle)
    {
        if (Theme)
        {
            switch (InStyle)
            {
            case EMAButtonStyle::Primary: return Theme->PrimaryColor;
            case EMAButtonStyle::Secondary: return Theme->SecondaryColor;
            case EMAButtonStyle::Danger: return Theme->DangerColor;
            case EMAButtonStyle::Success: return Theme->SuccessColor;
            case EMAButtonStyle::Warning: return Theme->WarningColor;
            }
        }

        switch (InStyle)
        {
        case EMAButtonStyle::Primary: return FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
        case EMAButtonStyle::Secondary: return FLinearColor(0.3f, 0.3f, 0.35f, 1.0f);
        case EMAButtonStyle::Danger: return FLinearColor(1.0f, 0.3f, 0.3f, 1.0f);
        case EMAButtonStyle::Success: return FLinearColor(0.3f, 0.8f, 0.4f, 1.0f);
        case EMAButtonStyle::Warning: return FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);
        }
        return FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
    };

    Palette.BaseColor = ResolveBase(Style);
    Palette.HoverColor = FLinearColor(
        FMath::Min(Palette.BaseColor.R * 1.2f, 1.0f),
        FMath::Min(Palette.BaseColor.G * 1.2f, 1.0f),
        FMath::Min(Palette.BaseColor.B * 1.2f, 1.0f),
        Palette.BaseColor.A
    );
    return Palette;
}

void FMAStyledButtonCoordinator::StepAnimation(FMAButtonAnimationState& Current, const FMAButtonAnimationState& Target, float DeltaTime, float InterpSpeed) const
{
    const float InterpAlpha = FMath::Clamp(DeltaTime * InterpSpeed, 0.0f, 1.0f);
    Current.PositionOffset = FMath::Lerp(Current.PositionOffset, Target.PositionOffset, InterpAlpha);
    Current.Scale = FMath::Lerp(Current.Scale, Target.Scale, InterpAlpha);
    Current.TintColor = FMath::Lerp(Current.TintColor, Target.TintColor, InterpAlpha);
    Current.ShadowOpacity = FMath::Lerp(Current.ShadowOpacity, Target.ShadowOpacity, InterpAlpha);
}
