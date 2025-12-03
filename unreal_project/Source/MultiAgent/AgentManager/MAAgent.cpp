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

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 160.0f, 0.0f);  // 降低自转速度（原 640）
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 150.f;
    GetCharacterMovement()->AvoidanceWeight = 0.5f;

    // 司能组件 (ASC)
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
    
    // 导航期间调用虚函数，子类可重写
    if (bIsMoving)
    {
        OnNavigationTick();
    }
}

void AMAAgent::OnNavigationTick()
{
    // 基类默认空实现，子类可重写
}

// ========== 司能 (GAS ASC Abilities) ==========

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

bool AMAAgent::TryNavigateTo(FVector Destination)
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateNavigate(Destination);
    }
    return false;
}

void AMAAgent::CancelNavigation()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelNavigate();
    }
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
