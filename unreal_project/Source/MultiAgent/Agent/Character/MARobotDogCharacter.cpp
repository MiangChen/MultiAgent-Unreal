// MARobotDogCharacter.cpp

#include "MARobotDogCharacter.h"
#include "Components/CapsuleComponent.h"
#include "StateTree.h"
#include "StateTreeTypes.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "MAAbilitySystemComponent.h"
#include "MAGameplayTags.h"
#include "MAStateTreeComponent.h"

AMARobotDogCharacter::AMARobotDogCharacter()
{
    AgentType = EMAAgentType::RobotDog;
    bIsPlayingWalk = false;
    
    // Energy defaults
    Energy = 100.f;
    MaxEnergy = 100.f;
    EnergyDrainRate = 1.f;
    
    // 创建 StateTree 组件 (使用自定义的 UMAStateTreeComponent)
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(true);
    
    // 注意: StateTree Asset 需要在蓝图中设置，或者在编辑器中选择 RobotDog 后设置
    // 路径: /Game/StateTree/ST_RobotDog.ST_RobotDog
    
    // CapsuleComponent 大小由基类 BeginPlay 中 AutoFitCapsuleToMesh() 自动计算
    
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
        TEXT("/Game/Robot/go1/go1.go1"));
    if (MeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
        GetMesh()->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
    }

    static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimAsset(
        TEXT("/Game/Robot/go1/1Idle.1Idle"));
    if (IdleAnimAsset.Succeeded())
    {
        IdleAnim = IdleAnimAsset.Object;
    }
    
    static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAnimAsset(
        TEXT("/Game/Robot/go1/1LYP.1LYP"));
    if (WalkAnimAsset.Succeeded())
    {
        WalkAnim = WalkAnimAsset.Object;
    }

    GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    if (IdleAnim)
    {
        GetMesh()->SetAnimation(IdleAnim);
        GetMesh()->Play(true);
    }
    
    GetCharacterMovement()->MaxWalkSpeed = 150.f;
    GetCharacterMovement()->MaxAcceleration = 512.f;
}

void AMARobotDogCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    // 动态加载 StateTree Asset
    if (StateTreeComponent && !StateTreeComponent->HasStateTree())
    {
        UStateTree* ST = LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_RobotDog.ST_RobotDog"));
        if (ST)
        {
            StateTreeComponent->SetStateTreeAsset(ST);
        }
    }
}

void AMARobotDogCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    float Speed = GetVelocity().Size();
    
    // Animation handling
    if (Speed > 10.f && !bIsPlayingWalk)
    {
        PlayWalkAnimation();
    }
    else if (Speed <= 10.f && bIsPlayingWalk)
    {
        PlayIdleAnimation();
    }
    
    // Energy drain while moving
    if (bIsMoving && HasEnergy())
    {
        DrainEnergy(DeltaTime);
    }
    
    // Update energy display and status
    UpdateEnergyDisplay();
    CheckLowEnergyStatus();
}

void AMARobotDogCharacter::PlayWalkAnimation()
{
    if (WalkAnim)
    {
        GetMesh()->SetAnimation(WalkAnim);
        GetMesh()->SetPlayRate(2.5f);
        GetMesh()->Play(true);
        bIsPlayingWalk = true;
    }
}

void AMARobotDogCharacter::PlayIdleAnimation()
{
    if (IdleAnim)
    {
        GetMesh()->SetAnimation(IdleAnim);
        GetMesh()->Play(true);
        bIsPlayingWalk = false;
    }
}

// ========== Energy System ==========

void AMARobotDogCharacter::DrainEnergy(float DeltaTime)
{
    Energy = FMath::Max(0.f, Energy - EnergyDrainRate * DeltaTime);
    
    // Stop movement if energy depleted
    if (Energy <= 0.f)
    {
        CancelNavigation();
        StopFollowing();
        UE_LOG(LogTemp, Warning, TEXT("[%s] Energy depleted! Stopping movement."), *AgentName);
    }
}

void AMARobotDogCharacter::RestoreEnergy(float Amount)
{
    float OldEnergy = Energy;
    // 如果 Amount >= MaxEnergy，直接设置为 MaxEnergy（充满）
    if (Amount >= MaxEnergy)
    {
        Energy = MaxEnergy;
    }
    else
    {
        Energy = FMath::Min(MaxEnergy, Energy + Amount);
    }
    UE_LOG(LogTemp, Warning, TEXT("[%s] Energy restored: %.1f -> %.1f (Amount: %.1f)"), 
        *AgentName, OldEnergy, Energy, Amount);
}

void AMARobotDogCharacter::UpdateEnergyDisplay()
{
    // Display energy above head
    FVector TextLocation = GetActorLocation() + FVector(0.f, 0.f, 120.f);
    FString EnergyText = FString::Printf(TEXT("Energy: %.0f%%"), GetEnergyPercent());
    
    FColor DisplayColor = FColor::Green;
    if (Energy < LowEnergyThreshold)
    {
        DisplayColor = FColor::Red;
    }
    else if (Energy < 50.f)
    {
        DisplayColor = FColor::Yellow;
    }
    
    DrawDebugString(
        GetWorld(),
        TextLocation,
        EnergyText,
        nullptr,
        DisplayColor,
        0.f,
        true,
        1.0f
    );
}

void AMARobotDogCharacter::CheckLowEnergyStatus()
{
    if (!AbilitySystemComponent) return;
    
    const FMAGameplayTags& Tags = FMAGameplayTags::Get();
    bool bHasLowEnergyTag = AbilitySystemComponent->HasGameplayTagFromContainer(Tags.Status_LowEnergy);
    
    if (Energy < LowEnergyThreshold && !bHasLowEnergyTag)
    {
        AbilitySystemComponent->AddLooseGameplayTag(Tags.Status_LowEnergy);
        UE_LOG(LogTemp, Warning, TEXT("[%s] Low energy warning! %.0f%%"), *AgentName, GetEnergyPercent());
    }
    else if (Energy >= LowEnergyThreshold && bHasLowEnergyTag)
    {
        AbilitySystemComponent->RemoveLooseGameplayTag(Tags.Status_LowEnergy);
    }
}

// ========== Robot Abilities ==========

bool AMARobotDogCharacter::TrySearch()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateSearch();
    }
    return false;
}

void AMARobotDogCharacter::StopSearch()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelSearch();
    }
}

bool AMARobotDogCharacter::TryCharge()
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->TryActivateCharge();
    }
    return false;
}
