// GA_Search.cpp

#include "GA_Search.h"
#include "../MAGameplayTags.h"
#include "../../Character/MACharacter.h"
#include "../../Character/MAHumanCharacter.h"
#include "../../Actor/MACameraSensor.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UGA_Search::UGA_Search()
{
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Search);
    SetAssetTags(AssetTags);
    
    ActivationOwnedTags.AddTag(FMAGameplayTags::Get().Status_Searching);
}

AMACameraSensor* UGA_Search::GetAttachedCamera() const
{
    AMACharacter* Character = const_cast<UGA_Search*>(this)->GetOwningCharacter();
    if (!Character) return nullptr;
    
    TArray<AActor*> AttachedActors;
    Character->GetAttachedActors(AttachedActors);
    
    for (AActor* Actor : AttachedActors)
    {
        if (AMACameraSensor* Camera = Cast<AMACameraSensor>(Actor))
        {
            return Camera;
        }
    }
    
    return nullptr;
}

bool UGA_Search::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    
    // Require attached camera sensor
    if (!GetAttachedCamera())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Search] No attached CameraSensor"));
        return false;
    }
    
    return true;
}

void UGA_Search::ActivateAbility(
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
        UE_LOG(LogTemp, Log, TEXT("[Search] %s starting search"), *Character->ActorName);
        Character->ShowAbilityStatus(TEXT("Search"), TEXT("Scanning..."));
        
        // Start periodic detection
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().SetTimer(
                SearchTimerHandle,
                this,
                &UGA_Search::PerformDetection,
                DetectionInterval,
                true,
                0.f  // Start immediately
            );
        }
    }
}

void UGA_Search::PerformDetection()
{
    AMAHumanCharacter* DetectedHuman = nullptr;
    
    if (DetectHumanInView(DetectedHuman))
    {
        OnTargetFound(DetectedHuman);
    }
}

bool UGA_Search::DetectHumanInView(AMAHumanCharacter*& OutHuman)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return false;
    
    UWorld* World = Character->GetWorld();
    if (!World) return false;
    
    // Get all Human characters
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAHumanCharacter::StaticClass(), FoundActors);
    
    FVector CharacterLocation = Character->GetActorLocation();
    FVector ForwardVector = Character->GetActorForwardVector();
    
    for (AActor* Actor : FoundActors)
    {
        AMAHumanCharacter* Human = Cast<AMAHumanCharacter>(Actor);
        if (!Human) continue;
        
        FVector ToHuman = Human->GetActorLocation() - CharacterLocation;
        float Distance = ToHuman.Size();
        
        // Check range
        if (Distance > DetectionRange) continue;
        
        // Check FOV
        ToHuman.Normalize();
        float DotProduct = FVector::DotProduct(ForwardVector, ToHuman);
        float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(DotProduct));
        
        if (AngleDegrees > DetectionFOV / 2.f) continue;
        
        // Line trace to check visibility
        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(Character);
        
        bool bHit = World->LineTraceSingleByChannel(
            HitResult,
            CharacterLocation + FVector(0.f, 0.f, 50.f),  // Eye level
            Human->GetActorLocation() + FVector(0.f, 0.f, 50.f),
            ECC_Visibility,
            QueryParams
        );
        
        // If we hit the human or nothing (clear line of sight)
        if (!bHit || HitResult.GetActor() == Human)
        {
            OutHuman = Human;
            return true;
        }
    }
    
    return false;
}

void UGA_Search::OnTargetFound(AMAHumanCharacter* Human)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !Human) return;
    
    // Output "Find Target"
    UE_LOG(LogTemp, Warning, TEXT("[Search] %s: Find Target! Detected %s"), 
        *Character->ActorName, *Human->ActorName);
    
    // Show on screen
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.f,
            FColor::Red,
            FString::Printf(TEXT("[%s] Find Target: %s"), *Character->ActorName, *Human->ActorName)
        );
    }
    
    // Update status display
    Character->ShowAbilityStatus(TEXT("Search"), 
        FString::Printf(TEXT("Found: %s"), *Human->ActorName));
    
    // Send gameplay event
    if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
    {
        FGameplayEventData EventData;
        EventData.Instigator = Character;
        EventData.Target = Human;
        ASC->HandleGameplayEvent(FMAGameplayTags::Get().Event_Target_Found, &EventData);
    }
}

void UGA_Search::CleanupSearch()
{
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->ShowStatus(TEXT(""), 0.f);
        
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(SearchTimerHandle);
        }
    }
}

void UGA_Search::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    CleanupSearch();
    
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        UE_LOG(LogTemp, Log, TEXT("[Search] %s search ended"), *Character->ActorName);
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
