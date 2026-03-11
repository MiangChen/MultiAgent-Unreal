#include "MAHUDBootstrap.h"

#include "UI/HUD/MAHUD.h"
#include "UI/Core/MAUIManager.h"
#include "UI/Core/Bootstrap/MAUIBootstrap.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDBootstrap, Log, All);

void FMAHUDBootstrap::InitializeHUD(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    const FMAUIBootstrap UIBootstrap;
    HUD->UIManager = UIBootstrap.CreateAndInitializeUIManager(HUD);
    HUD->ViewCoordinator.SetUIManager(HUD->UIManager);
    HUD->LifecycleCoordinator.BindRuntimeDelegates(HUD);
}
