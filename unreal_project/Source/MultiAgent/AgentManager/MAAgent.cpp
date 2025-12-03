#include "MAAgent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "../GAS/MAAbilitySystemComponent.h"
#include "../GAS/MAGameplayTags.h"
#include "../Interaction/MAPickupItem.h"

AMAAgent::AMAAgent()
{
    PrimaryActorTick.bCanEverTick = true;
    
    AgentID = 0;
    AgentName = TEXT("Agent");
    AgentType = EAgentType::Human;
    bIsMoving = false;
    TargetLocation = FVector::ZeroVector;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 150.f;
    GetCharacterMovement()->AvoidanceWeight = 0.5f;

    AbilitySystemComponent = CreateDefaultSubobject<UMAAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

UAbilitySystemComponent* AMAAgent::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AMAAgent::BeginPlay()
{
    Super::BeginPlay();
    FMAGameplayTags::InitializeNativeTags();
}

void AMAAgent::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        AbilitySystemComponent->InitializeAbilities(this);
    }
}


void AMAAgent::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateAnimation();
    
    if (bIsMoving)
    {
        float Distance = FVector::Dist2D(GetActorLocation(), TargetLocation);
        if (Distance < 100.f)
        {
            bIsMoving = false;
            StopMovement();
        }
    }
}

void AMAAgent::MoveToLocation(FVector Destination)
{
    TargetLocation = Destination;
    bIsMoving = true;
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->MoveToLocation(Destination);
    }
}

void AMAAgent::UpdateAnimation()
{
    if (!bIsMoving) return;
    
    FVector Velocity = GetCharacterMovement()->Velocity;
    float Speed = Velocity.Size2D();
    if (Speed > 3.0f)
    {
        FVector AccelDir = Velocity.GetSafeNormal2D();
        GetCharacterMovement()->AddInputVector(AccelDir);
    }
}

void AMAAgent::StopMovement()
{
    bIsMoving = false;
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->StopMovement();
    }
    GetCharacterMovement()->StopMovementImmediately();
}

FVector AMAAgent::GetCurrentLocation() const
{
    return GetActorLocation();
}

// ========== GAS Abilities ==========

bool AMAAgent::TryPickup()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivatePickup();
    }
    return false;
}

bool AMAAgent::TryDrop()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateDrop();
    }
    return false;
}

AMAPickupItem* AMAAgent::GetHeldItem() const
{
    TArray<AActor*> AttachedActors;
    GetAttachedActors(AttachedActors);
    for (AActor* Actor : AttachedActors)
    {
        if (AMAPickupItem* Item = Cast<AMAPickupItem>(Actor))
        {
            return Item;
        }
    }
    return nullptr;
}

bool AMAAgent::IsHoldingItem() const
{
    return GetHeldItem() != nullptr;
}
