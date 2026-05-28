#include "MANavigationBootstrap.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Core/Config/MAConfigManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void FMANavigationBootstrap::ApplyConfig(UMANavigationService& NavigationService, UWorld* World)
{
    if (!World)
    {
        return;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    UMAConfigManager* ConfigMgr = GameInstance->GetSubsystem<UMAConfigManager>();
    if (!ConfigMgr)
    {
        return;
    }

    NavigationService.ApplyBootstrapConfig(
        ConfigMgr->GetPathPlannerTypeEnum(),
        ConfigMgr->GetPathPlannerConfig(),
        ConfigMgr->GetFlightConfig().MinAltitude,
        ConfigMgr->GetFollowConfig().Distance,
        ConfigMgr->GetFollowConfig().PositionTolerance,
        ConfigMgr->GetGroundNavigationConfig().StuckTimeout
    );
}
