// GA_Charge.cpp

#include "GA_Charge.h"
#include "../MAGameplayTags.h"
#include "../../Character/MACharacter.h"
#include "../../Character/MARobotDogCharacter.h"
#include "../../Actor/MAChargingStation.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

UGA_Charge::UGA_Charge()
{
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Charge);
    SetAssetTags(AssetTags);
    
    ActivationOwnedTags.AddTag(FMAGameplayTags::Get().Status_Charging);
}

AMAChargingStation* UGA_Charge::FindNearbyChargingStation() const
{
    AMACharacter* Character = const_cast<UGA_Charge*>(this)->GetOwningCharacter();
    if (!Character)
    {
        return nullptr;
    }
    
    UWorld* World = Character->GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    // Find all charging stations
    TArray<AActor*> FoundStations;
    UGameplayStatics::GetAllActorsOfClass(World, AMAChargingStation::StaticClass(), FoundStations);
    
    for (AActor* Actor : FoundStations)
    {
        AMAChargingStation* Station = Cast<AMAChargingStation>(Actor);
        if (Station && Station->IsRobotInRange(Character))
        {
            return Station;
        }
    }
    
    return nullptr;
}

bool UGA_Charge::CanActivateAbility(
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
    
    // Must be a RobotDog
    AMACharacter* Character = const_cast<UGA_Charge*>(this)->GetOwningCharacter();
    if (!Cast<AMARobotDogCharacter>(Character))
    {
        return false;
    }
    
    // Must be in range of a charging station
    return FindNearbyChargingStation() != nullptr;
}

void UGA_Charge::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    PerformCharge();
}

void UGA_Charge::PerformCharge()
{
    AMACharacter* Character = GetOwningCharacter();
    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Character);
    
    if (!Robot)
    {
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // Restore energy to 100%
    Robot->RestoreEnergy(Robot->MaxEnergy);
    
    // Show status
    Robot->ShowAbilityStatus(TEXT("Charge"), TEXT("Complete!"));
    
    // Send gameplay event
    if (UAbilitySystemComponent* ASC = Robot->GetAbilitySystemComponent())
    {
        FGameplayEventData EventData;
        EventData.Instigator = Robot;
        ASC->HandleGameplayEvent(FMAGameplayTags::Get().Event_Charge_Complete, &EventData);
    }
    
    // End ability
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
}

void UGA_Charge::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
