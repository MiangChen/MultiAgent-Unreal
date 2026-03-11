#pragma once

#include "CoreMinimal.h"
#include "Core/Camera/Domain/MAPIPCameraTypes.h"

class UWorld;

struct MULTIAGENT_API FMASkillPIPCameraBridge
{
    static bool IsAvailable(UWorld* World);
    static FGuid CreatePIPCamera(UWorld* World, const FMAPIPCameraConfig& Config);
    static bool ShowPIPCamera(UWorld* World, const FGuid& CameraId, const FMAPIPDisplayConfig& DisplayConfig);
    static void HidePIPCamera(UWorld* World, const FGuid& CameraId);
    static void DestroyPIPCamera(UWorld* World, FGuid& InOutCameraId);
    static bool DoesPIPCameraExist(UWorld* World, const FGuid& CameraId);
    static FVector2D AllocateScreenPosition(UWorld* World, const FVector2D& Size, const FVector2D& FallbackPosition = FVector2D(0.7f, 0.3f));
};
