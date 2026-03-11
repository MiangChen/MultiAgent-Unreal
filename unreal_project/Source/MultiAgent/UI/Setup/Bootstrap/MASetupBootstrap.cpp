#include "MASetupBootstrap.h"

#include "UI/Setup/Runtime/MASetupHUD.h"

void FMASetupBootstrap::InitializeSetupHUD(AMASetupHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    if (!HUD->Coordinator.ShouldShowSetupUI(*HUD))
    {
        UE_LOG(LogTemp, Log, TEXT("[MASetupHUD] bUseSetupUI=false, skipping Setup UI"));
        return;
    }

    HUD->Coordinator.ScheduleWidgetCreation(*HUD, [WeakHUD = TWeakObjectPtr<AMASetupHUD>(HUD)]()
    {
        if (WeakHUD.IsValid())
        {
            WeakHUD->CreateSetupWidget();
        }
    });
}
