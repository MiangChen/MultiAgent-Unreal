// GA_Report.cpp

#include "GA_Report.h"
#include "../MAGameplayTags.h"
#include "../../Character/MACharacter.h"
#include "TimerManager.h"

// Static members
TArray<FString> UGA_Report::MessageQueue;
bool UGA_Report::bIsDisplayingMessage = false;

UGA_Report::UGA_Report()
{
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Report);
    SetAssetTags(AssetTags);
}

void UGA_Report::SetReportMessage(const FString& InMessage)
{
    ReportMessage = InMessage;
}

void UGA_Report::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    ShowReportDialog();
}

void UGA_Report::ShowReportDialog()
{
    AMACharacter* Character = GetOwningCharacter();
    FString SenderName = Character ? Character->ActorName : TEXT("Unknown");
    
    // Format the message
    FString FormattedMessage = FString::Printf(TEXT("[Report from %s]\n%s"), *SenderName, *ReportMessage);
    
    // Add to queue
    MessageQueue.Add(FormattedMessage);
    
    UE_LOG(LogTemp, Log, TEXT("[Report] %s: %s"), *SenderName, *ReportMessage);
    
    // Process queue if not already displaying
    if (!bIsDisplayingMessage)
    {
        ProcessMessageQueue();
    }
    
    // Set timer to end ability
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().SetTimer(
                DisplayTimerHandle,
                this,
                &UGA_Report::HideReportDialog,
                DisplayDuration,
                false
            );
        }
    }
}

void UGA_Report::ProcessMessageQueue()
{
    if (MessageQueue.Num() == 0)
    {
        bIsDisplayingMessage = false;
        return;
    }
    
    bIsDisplayingMessage = true;
    
    // Get next message
    FString Message = MessageQueue[0];
    MessageQueue.RemoveAt(0);
    
    // Display on screen using GEngine
    if (GEngine)
    {
        // Use a unique key for report messages
        GEngine->AddOnScreenDebugMessage(
            100,  // Key for report messages
            5.f,
            FColor::Cyan,
            Message
        );
    }
}

void UGA_Report::HideReportDialog()
{
    // Process next message in queue
    ProcessMessageQueue();
    
    // End ability
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
}

void UGA_Report::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(DisplayTimerHandle);
        }
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
