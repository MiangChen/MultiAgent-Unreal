// HUD shortcut coordination for player input.

#include "MAHUDShortcutCoordinator.h"

#include "../../UI/Core/MAHUDStateManager.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDShortcutCoordinator, Log, All);

UMAHUDStateManager* FMAHUDShortcutCoordinator::ResolveHUDStateManager(const APlayerController* PlayerController) const
{
    return HUDInputAdapter.ResolveHUDStateManager(PlayerController);
}

void FMAHUDShortcutCoordinator::HandleCheckTask(APlayerController* PlayerController) const
{
    if (UMAHUDStateManager* StateManager = ResolveHUDStateManager(PlayerController))
    {
        StateManager->HandleCheckTaskInput();
        return;
    }

    UE_LOG(LogMAHUDShortcutCoordinator, Warning, TEXT("HandleCheckTask: HUDStateManager not found"));
}

void FMAHUDShortcutCoordinator::HandleCheckSkill(APlayerController* PlayerController) const
{
    if (UMAHUDStateManager* StateManager = ResolveHUDStateManager(PlayerController))
    {
        StateManager->HandleCheckSkillInput();
        return;
    }

    UE_LOG(LogMAHUDShortcutCoordinator, Warning, TEXT("HandleCheckSkill: HUDStateManager not found"));
}

void FMAHUDShortcutCoordinator::HandleCheckDecision(APlayerController* PlayerController) const
{
    if (UMAHUDStateManager* StateManager = ResolveHUDStateManager(PlayerController))
    {
        StateManager->HandleCheckDecisionInput();
        return;
    }

    UE_LOG(LogMAHUDShortcutCoordinator, Warning, TEXT("HandleCheckDecision: HUDStateManager not found"));
}

void FMAHUDShortcutCoordinator::ToggleSystemLogPanel(APlayerController* PlayerController) const
{
    HUDInputAdapter.ToggleSystemLogPanel(PlayerController);
}

void FMAHUDShortcutCoordinator::TogglePreviewPanel(APlayerController* PlayerController) const
{
    HUDInputAdapter.TogglePreviewPanel(PlayerController);
}

void FMAHUDShortcutCoordinator::ToggleInstructionPanel(APlayerController* PlayerController) const
{
    HUDInputAdapter.ToggleInstructionPanel(PlayerController);
}

bool FMAHUDShortcutCoordinator::ToggleAgentHighlight(APlayerController* PlayerController) const
{
    return HUDInputAdapter.ToggleAgentHighlight(PlayerController);
}
