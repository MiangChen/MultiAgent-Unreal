// MAUIInputModeAdapter.cpp
// UI 输入模式适配器实现（L3 Infrastructure）

#include "MAUIInputModeAdapter.h"
#include "../MAUIManager.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SViewport.h"
#include "Engine/World.h"

void FMAUIInputModeAdapter::SetInputModeGameAndUI(UMAUIManager* UIManager, UUserWidget* Widget) const
{
    APlayerController* OwningPC = UIManager ? UIManager->GetOwningPlayerController() : nullptr;
    if (!UIManager || !OwningPC || !Widget)
    {
        return;
    }

    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(Widget->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    OwningPC->SetInputMode(InputMode);
    OwningPC->SetShowMouseCursor(true);
}

void FMAUIInputModeAdapter::SetInputModeGameOnly(UMAUIManager* UIManager) const
{
    APlayerController* OwningPC = UIManager ? UIManager->GetOwningPlayerController() : nullptr;
    if (!UIManager || !OwningPC)
    {
        return;
    }

    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
    }

    FInputModeGameOnly InputMode;
    OwningPC->SetInputMode(InputMode);
    OwningPC->SetShowMouseCursor(true);

    UWorld* World = UIManager->GetWorld();
    if (!World)
    {
        return;
    }

    if (UGameViewportClient* ViewportClient = World->GetGameViewport())
    {
        if (TSharedPtr<SViewport> ViewportWidget = ViewportClient->GetGameViewportWidget())
        {
            FSlateApplication::Get().SetKeyboardFocus(StaticCastSharedPtr<SWidget>(ViewportWidget));
        }
    }
}
