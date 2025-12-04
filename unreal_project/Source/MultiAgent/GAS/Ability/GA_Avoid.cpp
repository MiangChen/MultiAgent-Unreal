// GA_Avoid.cpp

#include "GA_Avoid.h"
#include "../MAGameplayTags.h"
#include "../../Character/MACharacter.h"
#include "AIController.h"
#include "TimerManager.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_Avoid::UGA_Avoid()
{
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Avoid);
    SetAssetTags(AssetTags);
    
    ActivationOwnedTags.AddTag(FMAGameplayTags::Get().Status_Avoiding);
}

void UGA_Avoid::SetDestination(FVector InDestination)
{
    OriginalDestination = InDestination;
}

void UGA_Avoid::ActivateAbility(
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
        UE_LOG(LogTemp, Log, TEXT("[Avoid] %s starting obstacle avoidance"), *Character->ActorName);
        
        // Start periodic obstacle checking
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().SetTimer(
                AvoidTimerHandle,
                this,
                &UGA_Avoid::CheckObstacles,
                CheckInterval,
                true,
                0.f
            );
        }
    }
}

void UGA_Avoid::CheckObstacles()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        CleanupAvoid();
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    UWorld* World = Character->GetWorld();
    if (!World) return;
    
    // Sphere overlap to find nearby obstacles
    TArray<AActor*> OverlappingActors;
    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(Character);
    
    // Use sphere overlap
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
    
    UKismetSystemLibrary::SphereOverlapActors(
        World,
        Character->GetActorLocation(),
        DetectionRadius,
        ObjectTypes,
        nullptr,
        ActorsToIgnore,
        OverlappingActors
    );
    
    if (OverlappingActors.Num() > 0)
    {
        FVector AvoidanceDir = CalculateAvoidanceVector(OverlappingActors);
        if (!AvoidanceDir.IsNearlyZero())
        {
            ApplyAvoidance(AvoidanceDir);
        }
    }
    else
    {
        // No obstacles - resume direct navigation if we have a destination
        if (!OriginalDestination.IsNearlyZero())
        {
            AAIController* AICtrl = Cast<AAIController>(Character->GetController());
            if (AICtrl)
            {
                float DistToGoal = FVector::Dist(Character->GetActorLocation(), OriginalDestination);
                if (DistToGoal < 100.f)
                {
                    // Reached destination
                    CleanupAvoid();
                    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
                }
            }
        }
    }
}

FVector UGA_Avoid::CalculateAvoidanceVector(const TArray<AActor*>& Obstacles)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return FVector::ZeroVector;
    
    FVector CharacterLocation = Character->GetActorLocation();
    FVector AvoidanceSum = FVector::ZeroVector;
    
    for (AActor* Obstacle : Obstacles)
    {
        if (!Obstacle) continue;
        
        FVector ToObstacle = Obstacle->GetActorLocation() - CharacterLocation;
        float Distance = ToObstacle.Size();
        
        if (Distance < KINDA_SMALL_NUMBER) continue;
        
        // Avoidance force inversely proportional to distance
        float AvoidanceForce = (DetectionRadius - Distance) / DetectionRadius;
        AvoidanceForce = FMath::Clamp(AvoidanceForce, 0.f, 1.f);
        
        // Push away from obstacle
        FVector AwayFromObstacle = -ToObstacle.GetSafeNormal();
        AvoidanceSum += AwayFromObstacle * AvoidanceForce;
    }
    
    // Normalize and apply strength
    if (!AvoidanceSum.IsNearlyZero())
    {
        AvoidanceSum.Normalize();
        AvoidanceSum *= AvoidanceStrength;
    }
    
    return AvoidanceSum;
}

void UGA_Avoid::ApplyAvoidance(FVector AvoidanceDir)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;
    
    AAIController* AICtrl = Cast<AAIController>(Character->GetController());
    if (!AICtrl) return;
    
    FVector CurrentLocation = Character->GetActorLocation();
    
    // Combine avoidance with direction to goal
    FVector ToGoal = FVector::ZeroVector;
    if (!OriginalDestination.IsNearlyZero())
    {
        ToGoal = (OriginalDestination - CurrentLocation).GetSafeNormal();
    }
    
    // Blend avoidance and goal direction
    FVector CombinedDir = (ToGoal + AvoidanceDir * 2.f).GetSafeNormal();
    
    // Calculate avoidance target
    FVector AvoidanceTarget = CurrentLocation + CombinedDir * 200.f;
    
    // Move to avoidance position
    Character->bIsMoving = true;
    AICtrl->MoveToLocation(AvoidanceTarget, 30.f, false, true, false, true);
    
    // Show status
    Character->ShowAbilityStatus(TEXT("Avoid"), TEXT("Avoiding obstacle"));
}

void UGA_Avoid::CleanupAvoid()
{
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->ShowStatus(TEXT(""), 0.f);
        
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(AvoidTimerHandle);
        }
    }
}

void UGA_Avoid::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    CleanupAvoid();
    
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        UE_LOG(LogTemp, Log, TEXT("[Avoid] %s avoidance ended"), *Character->ActorName);
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
