#include "Agent/Skill/Infrastructure/MASkillPIPCameraBridge.h"

#include "Core/Camera/Runtime/MAPIPCameraManager.h"
#include "Engine/World.h"

namespace
{
UMAPIPCameraManager* ResolveManager(UWorld* World)
{
    return World ? World->GetSubsystem<UMAPIPCameraManager>() : nullptr;
}
}

bool FMASkillPIPCameraBridge::IsAvailable(UWorld* World)
{
    return ResolveManager(World) != nullptr;
}

FGuid FMASkillPIPCameraBridge::CreatePIPCamera(UWorld* World, const FMAPIPCameraConfig& Config)
{
    if (UMAPIPCameraManager* Manager = ResolveManager(World))
    {
        return Manager->CreatePIPCamera(Config);
    }

    return FGuid();
}

bool FMASkillPIPCameraBridge::ShowPIPCamera(UWorld* World, const FGuid& CameraId, const FMAPIPDisplayConfig& DisplayConfig)
{
    if (UMAPIPCameraManager* Manager = ResolveManager(World))
    {
        return Manager->ShowPIPCamera(CameraId, DisplayConfig);
    }

    return false;
}

void FMASkillPIPCameraBridge::HidePIPCamera(UWorld* World, const FGuid& CameraId)
{
    if (UMAPIPCameraManager* Manager = ResolveManager(World))
    {
        Manager->HidePIPCamera(CameraId);
    }
}

void FMASkillPIPCameraBridge::DestroyPIPCamera(UWorld* World, FGuid& InOutCameraId)
{
    if (InOutCameraId.IsValid())
    {
        if (UMAPIPCameraManager* Manager = ResolveManager(World))
        {
            Manager->DestroyPIPCamera(InOutCameraId);
        }

        InOutCameraId.Invalidate();
    }
}

bool FMASkillPIPCameraBridge::DoesPIPCameraExist(UWorld* World, const FGuid& CameraId)
{
    if (UMAPIPCameraManager* Manager = ResolveManager(World))
    {
        return Manager->DoesPIPCameraExist(CameraId);
    }

    return false;
}

FVector2D FMASkillPIPCameraBridge::AllocateScreenPosition(
    UWorld* World,
    const FVector2D& Size,
    const FVector2D& FallbackPosition)
{
    if (UMAPIPCameraManager* Manager = ResolveManager(World))
    {
        return Manager->AllocateScreenPosition(Size);
    }

    return FallbackPosition;
}
