// GA_Observe.cpp

#include "GA_Observe.h"
#include "../MAGameplayTags.h"
#include "MACharacter.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"

UGA_Observe::UGA_Observe()
{
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Observe);
    SetAssetTags(AssetTags);
    
    ActivationOwnedTags.AddTag(FMAGameplayTags::Get().Status_Observing);
}

void UGA_Observe::SetObserveTarget(AActor* InTarget)
{
    ObserveTarget = InTarget;
}

void UGA_Observe::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        FString TargetName = ObserveTarget.IsValid() ? ObserveTarget->GetName() : TEXT("Area");
        UE_LOG(LogTemp, Log, TEXT("[Observe] %s starting observation of %s"), 
            *Character->ActorName, *TargetName);
        
        Character->ShowAbilityStatus(TEXT("Observe"), TargetName);
        
        // Start periodic observation updates
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().SetTimer(
                ObserveTimerHandle,
                this,
                &UGA_Observe::UpdateObservation,
                UpdateInterval,
                true
            );
        }
    }
}

void UGA_Observe::UpdateObservation()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        CleanupObserve();
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // If we have a specific target, check if it's still in range
    if (ObserveTarget.IsValid())
    {
        AActor* Target = ObserveTarget.Get();
        float Distance = FVector::Dist(Character->GetActorLocation(), Target->GetActorLocation());
        
        if (Distance > ObserveRange)
        {
            OnTargetLost();
            return;
        }
        
        // Update status with distance
        Character->ShowAbilityStatus(TEXT("Observe"), 
            FString::Printf(TEXT("%s (%.0fm)"), *Target->GetName(), Distance / 100.f));
    }
    else
    {
        // Area observation - just show status
        Character->ShowAbilityStatus(TEXT("Observe"), TEXT("Monitoring area"));
    }
}

void UGA_Observe::OnTargetLost()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;
    
    UE_LOG(LogTemp, Warning, TEXT("[Observe] %s: Target Lost!"), *Character->ActorName);
    
    // Show on screen
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.f,
            FColor::Yellow,
            FString::Printf(TEXT("[%s] Target Lost"), *Character->ActorName)
        );
    }
    
    // Send gameplay event
    if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
    {
        FGameplayEventData EventData;
        EventData.Instigator = Character;
        ASC->HandleGameplayEvent(FMAGameplayTags::Get().Event_Target_Lost, &EventData);
    }
    
    // End the ability
    CleanupObserve();
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
}

void UGA_Observe::CleanupObserve()
{
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->ShowStatus(TEXT(""), 0.f);
        
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(ObserveTimerHandle);
        }
    }
}

void UGA_Observe::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    CleanupObserve();
    
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        UE_LOG(LogTemp, Log, TEXT("[Observe] %s observation ended"), *Character->ActorName);
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
