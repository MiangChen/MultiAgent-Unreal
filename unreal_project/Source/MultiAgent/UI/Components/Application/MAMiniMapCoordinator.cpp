#include "MAMiniMapCoordinator.h"

#include "../Infrastructure/MAMiniMapRuntimeAdapter.h"
#include "Blueprint/UserWidget.h"

FMAMiniMapFrameModel FMAMiniMapCoordinator::BuildFrameModel(const UUserWidget* Context, const FMAMiniMapViewportConfig& Config) const
{
    const FMAMiniMapRuntimeAdapter RuntimeAdapter;
    const FMAMiniMapRuntimeSnapshot Snapshot = RuntimeAdapter.Capture(Context);

    FMAMiniMapFrameModel Model;
    for (const FMAMiniMapAgentSample& Sample : Snapshot.Agents)
    {
        FMAMiniMapAgentMarker Marker;
        Marker.Position = WorldToMiniMap(Config, Sample.WorldLocation);
        Marker.Color = Sample.Color;
        Marker.Size = Config.AgentIconSize;
        Model.AgentMarkers.Add(Marker);
    }

    if (Snapshot.Camera.bValid)
    {
        Model.CameraIndicator = BuildCameraIndicator(Config, Snapshot.Camera.WorldLocation, Snapshot.Camera.Rotation);
    }

    return Model;
}

FMAMiniMapCameraIndicatorModel FMAMiniMapCoordinator::BuildCameraIndicator(const FMAMiniMapViewportConfig& Config, const FVector& CameraLocation, const FRotator& CameraRotation) const
{
    FMAMiniMapCameraIndicatorModel Model;
    Model.bVisible = true;
    Model.Position = WorldToMiniMap(Config, CameraLocation);
    Model.IconSize = Config.CameraIconSize;
    Model.FOVLength = Config.CameraFOVLength;

    const float HalfFOV = Config.CameraFOVAngle / 2.0f;
    Model.LeftAngle = CameraRotation.Yaw - HalfFOV - 90.0f;
    Model.RightAngle = CameraRotation.Yaw + HalfFOV - 90.0f;
    return Model;
}

bool FMAMiniMapCoordinator::HandleMiniMapClick(const UUserWidget* Context, const FMAMiniMapViewportConfig& Config, const FVector2D& LocalPosition) const
{
    const FVector2D Center(Config.MiniMapSize / 2.0f, Config.MiniMapSize / 2.0f);
    if (FVector2D::Distance(LocalPosition, Center) > Config.MiniMapSize / 2.0f)
    {
        return false;
    }

    const FMAMiniMapRuntimeAdapter RuntimeAdapter;
    return RuntimeAdapter.MoveViewToWorldLocation(Context, MiniMapToWorld(Config, LocalPosition));
}

FVector2D FMAMiniMapCoordinator::WorldToMiniMap(const FMAMiniMapViewportConfig& Config, const FVector& WorldLocation) const
{
    float X = (WorldLocation.X - Config.WorldCenter.X + Config.WorldBounds.X / 2.0f) / Config.WorldBounds.X * Config.MiniMapSize;
    float Y = (WorldLocation.Y - Config.WorldCenter.Y + Config.WorldBounds.Y / 2.0f) / Config.WorldBounds.Y * Config.MiniMapSize;

    X = FMath::Clamp(X, 0.0f, Config.MiniMapSize);
    Y = FMath::Clamp(Y, 0.0f, Config.MiniMapSize);
    return FVector2D(X, Y);
}

FVector FMAMiniMapCoordinator::MiniMapToWorld(const FMAMiniMapViewportConfig& Config, const FVector2D& MiniMapLocation) const
{
    const float X = (MiniMapLocation.X / Config.MiniMapSize) * Config.WorldBounds.X - Config.WorldBounds.X / 2.0f + Config.WorldCenter.X;
    const float Y = (MiniMapLocation.Y / Config.MiniMapSize) * Config.WorldBounds.Y - Config.WorldBounds.Y / 2.0f + Config.WorldCenter.Y;
    return FVector(X, Y, 0.0f);
}
