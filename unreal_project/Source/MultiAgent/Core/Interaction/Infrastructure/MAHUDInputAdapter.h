#pragma once

#include "CoreMinimal.h"

class APlayerController;
class AMASelectionHUD;
class AMAHUD;
class UMAHUDStateManager;

class MULTIAGENT_API FMAHUDInputAdapter
{
public:
    AMAHUD* ResolveHUD(const APlayerController* PlayerController) const;
    AMASelectionHUD* ResolveSelectionHUD(const APlayerController* PlayerController) const;
    UMAHUDStateManager* ResolveHUDStateManager(const APlayerController* PlayerController) const;

    bool IsMouseOverPersistentUI(const APlayerController* PlayerController) const;
    bool IsMainUIVisible(const APlayerController* PlayerController) const;
    bool IsMouseOverEditWidget(const APlayerController* PlayerController) const;

    void ToggleSystemLogPanel(APlayerController* PlayerController) const;
    void TogglePreviewPanel(APlayerController* PlayerController) const;
    void ToggleInstructionPanel(APlayerController* PlayerController) const;

    bool ToggleAgentHighlight(APlayerController* PlayerController) const;

    void ShowModifyWidget(APlayerController* PlayerController) const;
    void HideModifyWidget(APlayerController* PlayerController) const;
    void ShowEditWidget(APlayerController* PlayerController) const;
    void HideEditWidget(APlayerController* PlayerController) const;

    void SyncSelectionBox(
        const APlayerController* PlayerController,
        bool bBoxSelecting,
        const FVector2D& BoxStart,
        const FVector2D& BoxEnd) const;
};
