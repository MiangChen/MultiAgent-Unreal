#pragma once

#include "CoreMinimal.h"

struct FMAFlightConfig;
class AActor;
class AMACharacter;
class UMACommandManager;
class UMAUIManager;
class UMASpeechBubbleWidget;

struct MULTIAGENT_API FMACharacterRuntimeBridge
{
    static bool ShouldDrainEnergy(const AActor* WorldContext);
    static UMACommandManager* ResolveCommandManager(const AActor* WorldContext);
    static bool IsExecutionPaused(const AActor* WorldContext);
    static void BindPauseStateChanged(AMACharacter& Character);
    static void UnbindPauseStateChanged(AMACharacter& Character);
    static UMAUIManager* ResolveUIManager(const AActor* WorldContext);
    static void ApplySpeechBubbleTheme(const AActor* WorldContext, UMASpeechBubbleWidget& Widget);
    static bool ResolveFlightConfig(const AActor* WorldContext, FMAFlightConfig& OutFlightConfig);
};
