// Bootstrap wiring for interaction entry points.

#include "MAInteractionBootstrap.h"

#include "../../../Input/MAPlayerController.h"
#include "../../../Input/MAInputActions.h"
#include "EnhancedInputSubsystems.h"

void FMAInteractionBootstrap::InitializePlayerController(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    PlayerController->SetInputMode(InputMode);
    PlayerController->SetIgnoreLookInput(true);

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
    {
        if (UMAInputActions* InputActions = UMAInputActions::Get())
        {
            if (InputActions->DefaultMappingContext)
            {
                Subsystem->AddMappingContext(InputActions->DefaultMappingContext, 0);
            }
        }
    }
}
