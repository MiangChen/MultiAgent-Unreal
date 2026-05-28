#pragma once

#include "CoreMinimal.h"

struct FMAFlightConfig;
class AActor;
class AMACharacter;
class UMACommandManager;
class UMAUIManager;
class UMASpeechBubbleWidget;
class UWidgetComponent;

struct MULTIAGENT_API FMACharacterRuntimeBridge
{
    static bool ShouldDrainEnergy(const AActor* WorldContext);
    static UMACommandManager* ResolveCommandManager(const AActor* WorldContext);
    static bool IsExecutionPaused(const AActor* WorldContext);
    static void BindPauseStateChanged(AMACharacter& Character);
    static void UnbindPauseStateChanged(AMACharacter& Character);
    static void ConfigureSpeechBubbleComponent(UWidgetComponent& Component);
    static UMAUIManager* ResolveUIManager(const AActor* WorldContext);
    static void ApplySpeechBubbleTheme(const AActor* WorldContext, UMASpeechBubbleWidget& Widget);
    static void InitializeSpeechBubble(const AActor* WorldContext, UWidgetComponent& Component);
    static void UpdateSpeechBubbleFacing(const AActor* WorldContext, UWidgetComponent& Component);
    static void ShowSpeechBubble(UWidgetComponent& Component, const FString& Message, float Duration);
    static void HideSpeechBubble(UWidgetComponent& Component);
    static UMASpeechBubbleWidget* ResolveSpeechBubbleWidget(const UWidgetComponent* Component);
    static bool ResolveFlightConfig(const AActor* WorldContext, FMAFlightConfig& OutFlightConfig);
};
