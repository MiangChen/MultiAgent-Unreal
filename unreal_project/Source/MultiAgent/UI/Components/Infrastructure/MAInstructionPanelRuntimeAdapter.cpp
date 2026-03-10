#include "MAInstructionPanelRuntimeAdapter.h"

#include "../../Core/MAUIManager.h"
#include "../../HUD/MAHUD.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void FMAInstructionPanelRuntimeAdapter::DismissRequestNotification(const UUserWidget* Context) const
{
    if (!Context)
    {
        return;
    }

    UWorld* World = Context->GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }

    AMAHUD* HUD = Cast<AMAHUD>(PC->GetHUD());
    UMAUIManager* UIManager = HUD ? HUD->GetUIManager() : nullptr;
    if (UIManager)
    {
        UIManager->DismissRequestUserCommandNotification();
    }
}
