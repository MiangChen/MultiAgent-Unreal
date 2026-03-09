// HUD lifecycle coordination.

#include "MAHUDLifecycleCoordinator.h"
#include "../MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDLifecycleCoordinator, Log, All);

void FMAHUDLifecycleCoordinator::InitializeUI(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    APlayerController* PlayerController = HUD->GetOwningPlayerController();
    if (!PlayerController)
    {
        UE_LOG(LogMAHUDLifecycleCoordinator, Error, TEXT("InitializeUI: No owning PlayerController"));
        return;
    }

    HUD->UIManager = NewObject<UMAUIManager>(HUD);
    if (!HUD->UIManager)
    {
        UE_LOG(LogMAHUDLifecycleCoordinator, Error, TEXT("InitializeUI: Failed to create UIManager"));
        return;
    }

    HUD->UIManager->SemanticMapWidgetClass = HUD->SemanticMapWidgetClass;
    HUD->UIManager->Initialize(PlayerController);
    HUD->UIManager->LoadTheme(HUD->UIThemeAsset);
    HUD->UIManager->CreateAllWidgets();
}

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
