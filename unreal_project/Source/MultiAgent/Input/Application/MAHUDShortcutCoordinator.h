#pragma once

#include "CoreMinimal.h"
#include "../Infrastructure/MAHUDInputAdapter.h"

class APlayerController;
class UMAHUDStateManager;

class MULTIAGENT_API FMAHUDShortcutCoordinator
{
public:
    UMAHUDStateManager* ResolveHUDStateManager(const APlayerController* PlayerController) const;

    void HandleCheckTask(APlayerController* PlayerController) const;
    void HandleCheckSkill(APlayerController* PlayerController) const;
    void HandleCheckDecision(APlayerController* PlayerController) const;

    void ToggleSystemLogPanel(APlayerController* PlayerController) const;
    void TogglePreviewPanel(APlayerController* PlayerController) const;
    void ToggleInstructionPanel(APlayerController* PlayerController) const;
    bool ToggleAgentHighlight(APlayerController* PlayerController) const;

private:
    FMAHUDInputAdapter HUDInputAdapter;
};
