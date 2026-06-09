#pragma once

#include "CoreMinimal.h"

class AMACharacter;

struct MULTIAGENT_API FMASkillConfigBridge
{
    static void ApplyFollowConfig(
        const AMACharacter& Character,
        float& InOutFollowDistance,
        float& InOutPositionTolerance,
        float& InOutContinuousFollowTimeThreshold);

    static void ApplyGuideConfig(
        const AMACharacter& Character,
        float& InOutFollowDistance,
        float& InOutAcceptanceRadius,
        float& InOutWaitDistanceThreshold,
        bool& bOutUseNavMeshTargetMode);

    static void ApplyHandleHazardConfig(
        const AMACharacter& Character,
        float& InOutSafeDistance,
        float& InOutDuration,
        float& InOutSpraySpeed,
        float& InOutSprayWidth);

    static void ApplyTakePhotoConfig(
        const AMACharacter& Character,
        float& InOutPhotoDistance,
        float& InOutPhotoDuration,
        float& InOutCameraFOV,
        float& InOutCameraForwardOffset);

    static void ApplyBroadcastConfig(
        const AMACharacter& Character,
        float& InOutBroadcastDistance,
        float& InOutBroadcastDuration,
        float& InOutEffectSpeed,
        float& InOutEffectWidth,
        float& InOutEffectRate);

    static void ApplyClearConfig(
        const AMACharacter& Character,
        float& InOutStandoffDistance,
        float& InOutSpraySpeed,
        float& InOutSprayWidth,
        float& InOutMoveSpeed,
        float& InOutAcceptanceRadius);

    static void ApplyTransportConfig(
        const AMACharacter& Character,
        float& InOutGraspHeightOffset,
        float& InOutLiftAltitude,
        float& InOutCarryAltitude,
        float& InOutAcceptanceRadius,
        float& InOutReadyTimeout);
};
