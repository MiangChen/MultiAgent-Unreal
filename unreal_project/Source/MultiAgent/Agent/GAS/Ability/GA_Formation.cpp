// GA_Formation.cpp

#include "GA_Formation.h"
#include "../MAGameplayTags.h"
#include "MACharacter.h"
#include "MARobotDogCharacter.h"
#include "AIController.h"
#include "TimerManager.h"

UGA_Formation::UGA_Formation()
{
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Formation);
    SetAssetTags(AssetTags);
    
    ActivationOwnedTags.AddTag(FMAGameplayTags::Get().Status_InFormation);
    ActivationOwnedTags.AddTag(FMAGameplayTags::Get().Status_Moving);
}

void UGA_Formation::SetFormation(AMACharacter* InLeader, EFormationType InType, int32 InPosition, int32 InTotalCount)
{
    Leader = InLeader;
    FormationType = InType;
    FormationPosition = InPosition;
    TotalRobotCount = FMath::Max(1, InTotalCount);
}

bool UGA_Formation::CanActivateAbility(
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
    
    // Need a valid leader
    if (!Leader.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Formation] No valid leader"));
        return false;
    }
    
    // Check for AI Controller
    AMACharacter* Character = const_cast<UGA_Formation*>(this)->GetOwningCharacter();
    if (!Character) return false;
    
    AAIController* AICtrl = Cast<AAIController>(Character->GetController());
    if (!AICtrl)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Formation] No AIController"));
        return false;
    }
    
    // Check energy for RobotDog
    if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Character))
    {
        if (!Robot->HasEnergy())
        {
            UE_LOG(LogTemp, Warning, TEXT("[Formation] No energy"));
            return false;
        }
    }
    
    return true;
}

void UGA_Formation::ActivateAbility(
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
        FString FormationName;
        switch (FormationType)
        {
            case EFormationType::Line: FormationName = TEXT("Line"); break;
            case EFormationType::Column: FormationName = TEXT("Column"); break;
            case EFormationType::Wedge: FormationName = TEXT("Wedge"); break;
            case EFormationType::Diamond: FormationName = TEXT("X"); break;
            case EFormationType::Circle: FormationName = TEXT("Circle"); break;
        }
        
        UE_LOG(LogTemp, Log, TEXT("[Formation] %s joining %s formation, position %d"), 
            *Character->AgentName, *FormationName, FormationPosition);
        
        Character->ShowAbilityStatus(TEXT("Formation"), 
            FString::Printf(TEXT("%s #%d"), *FormationName, FormationPosition));
        
        // Start periodic position updates
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().SetTimer(
                FormationTimerHandle,
                this,
                &UGA_Formation::UpdateFormationPosition,
                UpdateInterval,
                true,
                0.f
            );
        }
    }
}

FVector UGA_Formation::CalculateFormationOffset() const
{
    // Calculate offset based on formation type and position
    FVector Offset = FVector::ZeroVector;
    int32 Pos = FormationPosition;
    
    switch (FormationType)
    {
        case EFormationType::Line:
            // Horizontal line: positions spread left/right
            // Position 0 = right of leader, 1 = left, 2 = further right, etc.
            if (Pos % 2 == 0)
                Offset = FVector(0.f, FormationSpacing * ((Pos / 2) + 1), 0.f);
            else
                Offset = FVector(0.f, -FormationSpacing * ((Pos / 2) + 1), 0.f);
            break;
            
        case EFormationType::Column:
            // Vertical column: positions behind leader
            Offset = FVector(-FormationSpacing * (Pos + 1), 0.f, 0.f);
            break;
            
        case EFormationType::Wedge:
            // V-shape: alternating left/right behind leader
            {
                int32 Row = (Pos / 2) + 1;
                float Side = (Pos % 2 == 0) ? 1.f : -1.f;
                Offset = FVector(-FormationSpacing * Row, FormationSpacing * Row * Side * 0.7f, 0.f);
            }
            break;
            
        case EFormationType::Diamond:
            // Diamond shape (X)
            switch (Pos % 4)
            {
                case 0: Offset = FVector(-FormationSpacing, 0.f, 0.f); break;  // Behind
                case 1: Offset = FVector(0.f, FormationSpacing, 0.f); break;   // Right
                case 2: Offset = FVector(0.f, -FormationSpacing, 0.f); break;  // Left
                case 3: Offset = FVector(FormationSpacing, 0.f, 0.f); break;   // Front
            }
            // Scale for outer rings
            if (Pos >= 4)
            {
                Offset *= (Pos / 4) + 1;
            }
            break;
            
        case EFormationType::Circle:
            // Circle: positions evenly distributed around leader
            // 半径根据机器人数量动态计算，确保机器人之间有足够间距
            {
                // 计算合适的半径：周长 = 机器人数量 * 最小间距
                // 半径 = 周长 / (2 * PI) = (数量 * 间距) / (2 * PI)
                float MinSpacing = FormationSpacing * 0.8f;  // 机器人之间的最小间距
                float Radius = FMath::Max(FormationSpacing, (TotalRobotCount * MinSpacing) / (2.f * PI));
                
                // 所有机器人均匀分布在圆上
                float Angle = (static_cast<float>(Pos) / static_cast<float>(TotalRobotCount)) * 2.f * PI;
                Offset = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
            }
            break;
    }
    
    return Offset;
}

void UGA_Formation::UpdateFormationPosition()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        CleanupFormation();
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // Check if leader is still valid
    if (!Leader.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Formation] %s: Leader lost!"), *Character->AgentName);
        CleanupFormation();
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // Check energy
    if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Character))
    {
        if (!Robot->HasEnergy())
        {
            UE_LOG(LogTemp, Warning, TEXT("[Formation] %s ran out of energy"), *Character->AgentName);
            CleanupFormation();
            EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
            return;
        }
    }
    
    AMACharacter* LeaderChar = Leader.Get();
    
    // Calculate target position in world space
    FVector LeaderLocation = LeaderChar->GetActorLocation();
    FRotator LeaderRotation = LeaderChar->GetActorRotation();
    FVector LocalOffset = CalculateFormationOffset();
    FVector WorldOffset = LeaderRotation.RotateVector(LocalOffset);
    FVector TargetLocation = LeaderLocation + WorldOffset;
    
    // Move to formation position
    AAIController* AICtrl = Cast<AAIController>(Character->GetController());
    if (AICtrl)
    {
        Character->bIsMoving = true;
        AICtrl->MoveToLocation(TargetLocation, 50.f, false, true, false, true);
    }
}

void UGA_Formation::CleanupFormation()
{
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->bIsMoving = false;
        Character->ShowStatus(TEXT(""), 0.f);
        
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(FormationTimerHandle);
        }
        
        if (AAIController* AICtrl = Cast<AAIController>(Character->GetController()))
        {
            AICtrl->StopMovement();
        }
    }
}

void UGA_Formation::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    CleanupFormation();
    
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        UE_LOG(LogTemp, Log, TEXT("[Formation] %s left formation"), *Character->AgentName);
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
