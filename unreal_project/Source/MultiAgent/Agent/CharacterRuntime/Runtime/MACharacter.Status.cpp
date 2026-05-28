#include "MACharacter.h"

#include "Agent/CharacterRuntime/Infrastructure/MACharacterRuntimeBridge.h"
#include "Components/WidgetComponent.h"

void AMACharacter::ShowStatus(const FString& Text, float Duration)
{
    CurrentStatusText = Text;
    StatusDisplayEndTime = GetWorld()->GetTimeSeconds() + Duration;
}

void AMACharacter::ShowAbilityStatus(const FString& SkillName, const FString& Params)
{
    const FString DisplayText = Params.IsEmpty()
        ? FString::Printf(TEXT("[%s]"), *SkillName)
        : FString::Printf(TEXT("[%s] %s"), *SkillName, *Params);
    ShowStatus(DisplayText, 3.0f);
}

FString AMACharacter::GetCurrentStatusText() const
{
    return CurrentStatusText;
}

void AMACharacter::UpdateStatusText()
{
    if (CurrentStatusText.IsEmpty()) return;
    if (GetWorld()->GetTimeSeconds() > StatusDisplayEndTime)
    {
        CurrentStatusText = TEXT("");
    }
}

void AMACharacter::UpdateSpeechBubbleFacing()
{
    if (!SpeechBubbleComponent) return;
    FMACharacterRuntimeBridge::UpdateSpeechBubbleFacing(this, *SpeechBubbleComponent);
}

void AMACharacter::InitializeSpeechBubble()
{
    if (!SpeechBubbleComponent) return;
    FMACharacterRuntimeBridge::InitializeSpeechBubble(this, *SpeechBubbleComponent);
}

void AMACharacter::ShowSpeechBubble(const FString& Message, float Duration)
{
    if (!SpeechBubbleComponent) return;
    FMACharacterRuntimeBridge::ShowSpeechBubble(*SpeechBubbleComponent, Message, Duration);
}

void AMACharacter::HideSpeechBubble()
{
    if (!SpeechBubbleComponent) return;
    FMACharacterRuntimeBridge::HideSpeechBubble(*SpeechBubbleComponent);
}

UMASpeechBubbleWidget* AMACharacter::GetSpeechBubbleWidget() const
{
    return FMACharacterRuntimeBridge::ResolveSpeechBubbleWidget(SpeechBubbleComponent);
}
