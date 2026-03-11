#include "MASetupRuntimeAdapter.h"

#include "../Domain/MASetupCatalog.h"
#include "../../../Core/Config/MAConfigManager.h"
#include "../../../Core/GameFlow/Bootstrap/MAGameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

bool FMASetupRuntimeAdapter::ShouldShowSetupUI(const AHUD* HUD) const
{
    if (!HUD)
    {
        return false;
    }

    UGameInstance* GameInstance = HUD->GetGameInstance();
    UMAConfigManager* ConfigMgr = GameInstance ? GameInstance->GetSubsystem<UMAConfigManager>() : nullptr;
    return ConfigMgr && ConfigMgr->bUseSetupUI;
}

void FMASetupRuntimeAdapter::ScheduleNextTick(AHUD* HUD, TFunction<void()> Callback) const
{
    if (!HUD || !HUD->GetWorld())
    {
        return;
    }

    HUD->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakHUD = TWeakObjectPtr<AHUD>(HUD), Callback = MoveTemp(Callback)]() mutable
    {
        if (WeakHUD.IsValid() && Callback)
        {
            Callback();
        }
    });
}

void FMASetupRuntimeAdapter::ApplySetupInputMode(APlayerController* PlayerController, UUserWidget* FocusWidget) const
{
    if (!PlayerController || !FocusWidget)
    {
        return;
    }

    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    PlayerController->SetInputMode(InputMode);
    PlayerController->bShowMouseCursor = true;
}

bool FMASetupRuntimeAdapter::SaveLaunchRequest(const AHUD* HUD, const FMASetupLaunchRequest& Request) const
{
    if (!HUD)
    {
        return false;
    }

    UMAGameInstance* GameInstance = Cast<UMAGameInstance>(HUD->GetGameInstance());
    if (!GameInstance)
    {
        return false;
    }

    GameInstance->SaveSetupConfig(Request.AgentConfigs, Request.SelectedScene);
    return true;
}

bool FMASetupRuntimeAdapter::OpenSelectedScene(const AHUD* HUD, const FString& SelectedScene) const
{
    if (!HUD)
    {
        return false;
    }

    FString MapPath = TEXT("/Game/Maps/") + SelectedScene;
    if (const FString* FoundPath = FMASetupCatalog::GetSceneToMapPaths().Find(SelectedScene))
    {
        MapPath = *FoundPath;
    }

    const FString Options = TEXT("?game=/Script/MultiAgent.MAGameMode");
    UGameplayStatics::OpenLevel(HUD, FName(*MapPath), true, Options);
    return true;
}
