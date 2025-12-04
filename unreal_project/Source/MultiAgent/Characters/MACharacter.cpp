// MACharacter.cpp

#include "MACharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "../GAS/MAAbilitySystemComponent.h"
#include "../GAS/MAGameplayTags.h"
#include "../Actors/MAPickupItem.h"

AMACharacter::AMACharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    
    ActorID = 0;
    ActorName = TEXT("Character");
    ActorType = EMAActorType::Human;
    bIsMoving = false;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 160.0f, 0.0f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 150.f;
    GetCharacterMovement()->AvoidanceWeight = 0.5f;

    AbilitySystemComponent = CreateDefaultSubobject<UMAAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

UAbilitySystemComponent* AMACharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AMACharacter::BeginPlay()
{
    Super::BeginPlay();
    FMAGameplayTags::InitializeNativeTags();
}

void AMACharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        AbilitySystemComponent->InitializeAbilities(this);
    }
}

void AMACharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (bIsMoving)
    {
        OnNavigationTick();
    }
    
    DrawStatusText();
}

void AMACharacter::OnNavigationTick()
{
}

// ========== 技能 (GAS Abilities) ==========

bool AMACharacter::TryPickup()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivatePickup();
    }
    return false;
}

bool AMACharacter::TryDrop()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateDrop();
    }
    return false;
}

bool AMACharacter::TryNavigateTo(FVector Destination)
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateNavigate(Destination);
    }
    return false;
}

void AMACharacter::CancelNavigation()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelNavigate();
    }
}

AMAPickupItem* AMACharacter::GetHeldItem() const
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

bool AMACharacter::IsHoldingItem() const
{
    return GetHeldItem() != nullptr;
}

bool AMACharacter::TryFollowActor(AMACharacter* TargetActor, float FollowDistance)
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateFollow(TargetActor, FollowDistance);
    }
    return false;
}

void AMACharacter::StopFollowing()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelFollow();
    }
}

// ========== 头顶状态显示 ==========

void AMACharacter::ShowStatus(const FString& Text, float Duration)
{
    if (!bShowStatusAboveHead) return;
    
    CurrentStatusText = Text;
    StatusDisplayEndTime = GetWorld()->GetTimeSeconds() + Duration;
}

void AMACharacter::ShowAbilityStatus(const FString& AbilityName, const FString& Params)
{
    FString DisplayText;
    if (Params.IsEmpty())
    {
        DisplayText = FString::Printf(TEXT("[%s]"), *AbilityName);
    }
    else
    {
        DisplayText = FString::Printf(TEXT("[%s] %s"), *AbilityName, *Params);
    }
    
    ShowStatus(DisplayText, 3.0f);
}

void AMACharacter::DrawStatusText()
{
    if (!bShowStatusAboveHead) return;
    if (CurrentStatusText.IsEmpty()) return;
    
    if (GetWorld()->GetTimeSeconds() > StatusDisplayEndTime)
    {
        CurrentStatusText = TEXT("");
        return;
    }
    
    FVector TextLocation = GetActorLocation() + FVector(0.f, 0.f, 150.f);
    
    DrawDebugString(
        GetWorld(),
        TextLocation,
        CurrentStatusText,
        nullptr,
        FColor::Yellow,
        0.f,
        true,
        1.2f
    );
}
