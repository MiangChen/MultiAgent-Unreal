#pragma once

#include "CoreMinimal.h"

struct FMAMiniMapViewportConfig
{
    float MiniMapSize = 200.0f;
    FVector2D WorldBounds = FVector2D(20000.0f, 20000.0f);
    FVector2D WorldCenter = FVector2D::ZeroVector;
    float AgentIconSize = 8.0f;
    float CameraIconSize = 12.0f;
    float CameraFOVLength = 30.0f;
    float CameraFOVAngle = 60.0f;
};

struct FMAMiniMapAgentSample
{
    FVector WorldLocation = FVector::ZeroVector;
    FLinearColor Color = FLinearColor::White;
};

struct FMAMiniMapCameraSample
{
    bool bValid = false;
    FVector WorldLocation = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
};

struct FMAMiniMapRuntimeSnapshot
{
    TArray<FMAMiniMapAgentSample> Agents;
    FMAMiniMapCameraSample Camera;
};

struct FMAMiniMapAgentMarker
{
    FVector2D Position = FVector2D::ZeroVector;
    FLinearColor Color = FLinearColor::White;
    float Size = 8.0f;
};

struct FMAMiniMapCameraIndicatorModel
{
    bool bVisible = false;
    FVector2D Position = FVector2D::ZeroVector;
    float LeftAngle = 0.0f;
    float RightAngle = 0.0f;
    float IconSize = 12.0f;
    float FOVLength = 30.0f;
};

struct FMAMiniMapFrameModel
{
    TArray<FMAMiniMapAgentMarker> AgentMarkers;
    FMAMiniMapCameraIndicatorModel CameraIndicator;
};
