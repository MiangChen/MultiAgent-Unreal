#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAMiniMapModels.h"

class UUserWidget;

class FMAMiniMapCoordinator
{
public:
    FMAMiniMapFrameModel BuildFrameModel(const UUserWidget* Context, const FMAMiniMapViewportConfig& Config) const;
    FMAMiniMapCameraIndicatorModel BuildCameraIndicator(const FMAMiniMapViewportConfig& Config, const FVector& CameraLocation, const FRotator& CameraRotation) const;
    bool HandleMiniMapClick(const UUserWidget* Context, const FMAMiniMapViewportConfig& Config, const FVector2D& LocalPosition) const;

private:
    FVector2D WorldToMiniMap(const FMAMiniMapViewportConfig& Config, const FVector& WorldLocation) const;
    FVector MiniMapToWorld(const FMAMiniMapViewportConfig& Config, const FVector2D& MiniMapLocation) const;
};
