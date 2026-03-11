#include "Agent/Skill/Infrastructure/MASkillConfigBridge.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Core/Config/MAConfigManager.h"

namespace
{
UMAConfigManager* ResolveConfigManager(const AMACharacter& Character)
{
    return Character.GetGameInstance()
        ? Character.GetGameInstance()->GetSubsystem<UMAConfigManager>()
        : nullptr;
}
}

void FMASkillConfigBridge::ApplyFollowConfig(
    const AMACharacter& Character,
    float& InOutFollowDistance,
    float& InOutPositionTolerance,
    float& InOutContinuousFollowTimeThreshold)
{
    if (const UMAConfigManager* ConfigManager = ResolveConfigManager(Character))
    {
        const FMAFollowConfig& Config = ConfigManager->GetFollowConfig();
        InOutFollowDistance = Config.Distance;
        InOutPositionTolerance = Config.PositionTolerance;
        InOutContinuousFollowTimeThreshold = Config.ContinuousTimeThreshold;
    }
}

void FMASkillConfigBridge::ApplyGuideConfig(
    const AMACharacter& Character,
    float& InOutFollowDistance,
    float& InOutAcceptanceRadius,
    float& InOutWaitDistanceThreshold,
    bool& bOutUseNavMeshTargetMode)
{
    if (const UMAConfigManager* ConfigManager = ResolveConfigManager(Character))
    {
        const FMAFollowConfig& FollowConfig = ConfigManager->GetFollowConfig();
        InOutFollowDistance = FollowConfig.Distance * 0.4f;

        const FMAGroundNavigationConfig& GroundConfig = ConfigManager->GetGroundNavigationConfig();
        InOutAcceptanceRadius = GroundConfig.AcceptanceRadius;

        const FMAGuideConfig& GuideConfig = ConfigManager->GetGuideConfig();
        InOutWaitDistanceThreshold = GuideConfig.WaitDistanceThreshold;
        bOutUseNavMeshTargetMode = !GuideConfig.TargetMoveMode.Equals(TEXT("direct"), ESearchCase::IgnoreCase);
    }
}

void FMASkillConfigBridge::ApplyHandleHazardConfig(
    const AMACharacter& Character,
    float& InOutSafeDistance,
    float& InOutDuration,
    float& InOutSpraySpeed,
    float& InOutSprayWidth)
{
    if (const UMAConfigManager* ConfigManager = ResolveConfigManager(Character))
    {
        const FMAHandleHazardConfig& Config = ConfigManager->GetHandleHazardConfig();
        InOutSafeDistance = Config.SafeDistance;
        InOutDuration = Config.Duration;
        InOutSpraySpeed = Config.SpraySpeed;
        InOutSprayWidth = Config.SprayWidth;
    }
}

void FMASkillConfigBridge::ApplyTakePhotoConfig(
    const AMACharacter& Character,
    float& InOutPhotoDistance,
    float& InOutPhotoDuration,
    float& InOutCameraFOV,
    float& InOutCameraForwardOffset)
{
    if (const UMAConfigManager* ConfigManager = ResolveConfigManager(Character))
    {
        const FMATakePhotoConfig& Config = ConfigManager->GetTakePhotoConfig();
        InOutPhotoDistance = Config.PhotoDistance;
        InOutPhotoDuration = Config.PhotoDuration;
        InOutCameraFOV = Config.CameraFOV;
        InOutCameraForwardOffset = Config.CameraForwardOffset;
    }
}

void FMASkillConfigBridge::ApplyBroadcastConfig(
    const AMACharacter& Character,
    float& InOutBroadcastDistance,
    float& InOutBroadcastDuration,
    float& InOutEffectSpeed,
    float& InOutEffectWidth,
    float& InOutEffectRate)
{
    if (const UMAConfigManager* ConfigManager = ResolveConfigManager(Character))
    {
        const FMABroadcastConfig& Config = ConfigManager->GetBroadcastConfig();
        InOutBroadcastDistance = Config.BroadcastDistance;
        InOutBroadcastDuration = Config.BroadcastDuration;
        InOutEffectSpeed = Config.EffectSpeed;
        InOutEffectWidth = Config.EffectWidth;
        InOutEffectRate = Config.ShockRate;
    }
}
