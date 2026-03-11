#include "Agent/CharacterRuntime/Infrastructure/MACharacterRuntimeBridge.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Core/Command/Runtime/MACommandManager.h"
#include "Core/Config/Domain/MAConfigNavigationTypes.h"
#include "Core/Config/MAConfigManager.h"
#include "UI/Components/Presentation/MASpeechBubbleWidget.h"
#include "UI/Core/MAUIManager.h"
#include "UI/HUD/Runtime/MAHUD.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
static UMAConfigManager* ResolveConfigManager(const AActor* WorldContext)
{
    if (const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UMAConfigManager>();
        }
    }

    return nullptr;
}
}

bool FMACharacterRuntimeBridge::ShouldDrainEnergy(const AActor* WorldContext)
{
    if (UMAConfigManager* ConfigManager = ResolveConfigManager(WorldContext))
    {
        return ConfigManager->bEnableEnergyDrain;
    }

    return false;
}

UMACommandManager* FMACharacterRuntimeBridge::ResolveCommandManager(const AActor* WorldContext)
{
    if (UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
    {
        return World->GetSubsystem<UMACommandManager>();
    }

    return nullptr;
}

bool FMACharacterRuntimeBridge::IsExecutionPaused(const AActor* WorldContext)
{
    if (UMACommandManager* CommandManager = ResolveCommandManager(WorldContext))
    {
        return CommandManager->IsPaused();
    }

    return false;
}

void FMACharacterRuntimeBridge::BindPauseStateChanged(AMACharacter& Character)
{
    if (UMACommandManager* CommandManager = ResolveCommandManager(&Character))
    {
        CommandManager->OnExecutionPauseStateChanged.AddDynamic(&Character, &AMACharacter::OnPauseStateChanged);
    }
}

void FMACharacterRuntimeBridge::UnbindPauseStateChanged(AMACharacter& Character)
{
    if (UMACommandManager* CommandManager = ResolveCommandManager(&Character))
    {
        CommandManager->OnExecutionPauseStateChanged.RemoveDynamic(&Character, &AMACharacter::OnPauseStateChanged);
    }
}

UMAUIManager* FMACharacterRuntimeBridge::ResolveUIManager(const AActor* WorldContext)
{
    if (UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AMAHUD* HUD = Cast<AMAHUD>(PC->GetHUD()))
            {
                return HUD->GetUIManager();
            }
        }
    }

    return nullptr;
}

void FMACharacterRuntimeBridge::ApplySpeechBubbleTheme(const AActor* WorldContext, UMASpeechBubbleWidget& Widget)
{
    if (UMAUIManager* UIManager = ResolveUIManager(WorldContext))
    {
        Widget.ApplyTheme(UIManager->GetTheme());
    }
}

bool FMACharacterRuntimeBridge::ResolveFlightConfig(const AActor* WorldContext, FMAFlightConfig& OutFlightConfig)
{
    if (UMAConfigManager* ConfigManager = ResolveConfigManager(WorldContext))
    {
        OutFlightConfig = ConfigManager->GetFlightConfig();
        return true;
    }

    return false;
}
