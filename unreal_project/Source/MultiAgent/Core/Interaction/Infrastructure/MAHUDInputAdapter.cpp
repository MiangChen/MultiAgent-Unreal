// HUD-facing adapter for input workflows.

#include "MAHUDInputAdapter.h"

#include "../../../UI/HUD/MAHUD.h"
#include "../../../UI/HUD/MASelectionHUD.h"
#include "../../../UI/Core/MAHUDStateManager.h"
#include "GameFramework/PlayerController.h"

AMAHUD* FMAHUDInputAdapter::ResolveHUD(const APlayerController* PlayerController) const
{
    return PlayerController ? Cast<AMAHUD>(PlayerController->GetHUD()) : nullptr;
}

AMASelectionHUD* FMAHUDInputAdapter::ResolveSelectionHUD(const APlayerController* PlayerController) const
{
    return PlayerController ? Cast<AMASelectionHUD>(PlayerController->GetHUD()) : nullptr;
}

UMAHUDStateManager* FMAHUDInputAdapter::ResolveHUDStateManager(const APlayerController* PlayerController) const
{
    if (const AMAHUD* HUD = ResolveHUD(PlayerController))
    {
        return HUD->GetHUDStateManager();
    }

    return nullptr;
}

bool FMAHUDInputAdapter::IsMouseOverPersistentUI(const APlayerController* PlayerController) const
{
    if (const AMAHUD* HUD = ResolveHUD(PlayerController))
    {
        return HUD->IsMouseOverPersistentUI();
    }

    return false;
}

bool FMAHUDInputAdapter::IsMainUIVisible(const APlayerController* PlayerController) const
{
    if (const AMAHUD* HUD = ResolveHUD(PlayerController))
    {
        return HUD->IsMainUIVisible();
    }

    return false;
}

bool FMAHUDInputAdapter::IsMouseOverEditWidget(const APlayerController* PlayerController) const
{
    if (const AMAHUD* HUD = ResolveHUD(PlayerController))
    {
        return HUD->IsMouseOverEditWidget();
    }

    return false;
}

void FMAHUDInputAdapter::ToggleSystemLogPanel(APlayerController* PlayerController) const
{
    if (AMAHUD* HUD = ResolveHUD(PlayerController))
    {
        HUD->ToggleSystemLogPanel();
    }
}

void FMAHUDInputAdapter::TogglePreviewPanel(APlayerController* PlayerController) const
{
    if (AMAHUD* HUD = ResolveHUD(PlayerController))
    {
        HUD->TogglePreviewPanel();
    }
}

void FMAHUDInputAdapter::ToggleInstructionPanel(APlayerController* PlayerController) const
{
    if (AMAHUD* HUD = ResolveHUD(PlayerController))
    {
        HUD->ToggleInstructionPanel();
    }
}

bool FMAHUDInputAdapter::ToggleAgentHighlight(APlayerController* PlayerController) const
{
    if (AMASelectionHUD* SelectionHUD = ResolveSelectionHUD(PlayerController))
    {
        SelectionHUD->ToggleAgentCircles();
        return SelectionHUD->IsAgentCirclesVisible();
    }

    return false;
}

void FMAHUDInputAdapter::ShowModifyWidget(APlayerController* PlayerController) const
{
    if (AMAHUD* HUD = ResolveHUD(PlayerController))
    {
        HUD->ShowModifyWidget();
    }
}

void FMAHUDInputAdapter::HideModifyWidget(APlayerController* PlayerController) const
{
    if (AMAHUD* HUD = ResolveHUD(PlayerController))
    {
        HUD->HideModifyWidget();
    }
}

void FMAHUDInputAdapter::ShowEditWidget(APlayerController* PlayerController) const
{
    if (AMAHUD* HUD = ResolveHUD(PlayerController))
    {
        HUD->ShowEditWidget();
    }
}

void FMAHUDInputAdapter::HideEditWidget(APlayerController* PlayerController) const
{
    if (AMAHUD* HUD = ResolveHUD(PlayerController))
    {
        HUD->HideEditWidget();
    }
}

void FMAHUDInputAdapter::SyncSelectionBox(
    const APlayerController* PlayerController,
    bool bBoxSelecting,
    const FVector2D& BoxStart,
    const FVector2D& BoxEnd) const
{
    if (AMASelectionHUD* SelectionHUD = ResolveSelectionHUD(PlayerController))
    {
        SelectionHUD->SetBoxSelecting(bBoxSelecting);
        SelectionHUD->SetBoxStart(BoxStart);
        SelectionHUD->SetBoxEnd(BoxEnd);
    }
}
