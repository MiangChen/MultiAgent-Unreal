#include "MASetupHUDCoordinator.h"

#include "../Infrastructure/MASetupRuntimeAdapter.h"
#include "../MASetupHUD.h"
#include "GameFramework/PlayerController.h"

bool FMASetupHUDCoordinator::ShouldShowSetupUI(const AMASetupHUD& HUD) const
{
    const FMASetupRuntimeAdapter RuntimeAdapter;
    return RuntimeAdapter.ShouldShowSetupUI(&HUD);
}

void FMASetupHUDCoordinator::ScheduleWidgetCreation(AMASetupHUD& HUD, TFunction<void()> Callback) const
{
    const FMASetupRuntimeAdapter RuntimeAdapter;
    RuntimeAdapter.ScheduleNextTick(&HUD, MoveTemp(Callback));
}

void FMASetupHUDCoordinator::ApplyWidgetInputMode(AMASetupHUD& HUD, UUserWidget* Widget) const
{
    const FMASetupRuntimeAdapter RuntimeAdapter;
    RuntimeAdapter.ApplySetupInputMode(HUD.GetOwningPlayerController(), Widget);
}

bool FMASetupHUDCoordinator::StartSimulation(AMASetupHUD& HUD, const FMASetupLaunchRequest& Request) const
{
    const FMASetupRuntimeAdapter RuntimeAdapter;
    return RuntimeAdapter.SaveLaunchRequest(&HUD, Request)
        && RuntimeAdapter.OpenSelectedScene(&HUD, Request.SelectedScene);
}
