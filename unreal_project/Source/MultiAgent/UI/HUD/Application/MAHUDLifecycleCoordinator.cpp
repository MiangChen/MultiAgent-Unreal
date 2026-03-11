// HUD lifecycle coordination.

#include "MAHUDLifecycleCoordinator.h"
#include "../MAHUD.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDLifecycleCoordinator, Log, All);

void FMAHUDLifecycleCoordinator::BindRuntimeDelegates(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    HUD->BindWidgetDelegates();
    HUD->BindControllerEvents();
    HUD->BindEditModeManagerEvents();
    HUD->BindEditWidgetDelegates();
    HUD->BindBackendEvents();
}
