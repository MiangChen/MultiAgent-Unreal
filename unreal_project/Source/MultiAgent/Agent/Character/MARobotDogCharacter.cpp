// MARobotDogCharacter.cpp

#include "MARobotDogCharacter.h"
#include "Components/CapsuleComponent.h"
#include "StateTree.h"
#include "StateTreeTypes.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "../GAS/MAAbilitySystemComponent.h"
#include "../GAS/MAGameplayTags.h"
#include "../StateTree/MAStateTreeComponent.h"
#include "../Component/Capability/MACapabilityComponents.h"

AMARobotDogCharacter::AMARobotDogCharacter()
{
    AgentType = EMAAgentType::RobotDog;
    bIsPlayingWalk = false;
    
    // ========== 创建 Capability Components ==========
    EnergyComponent = CreateDefaultSubobject<UMAEnergyComponent>(TEXT("EnergyComponent"));
    EnergyComponent->Energy = 100.f;
    EnergyComponent->MaxEnergy = 100.f;
    EnergyComponent->EnergyDrainRate = 1.f;

    PatrolComponent = CreateDefaultSubobject<UMAPatrolComponent>(TEXT("PatrolComponent"));
    PatrolComponent->ScanRadius = 200.f;

    FollowComponent = CreateDefaultSubobject<UMAFollowComponent>(TEXT("FollowComponent"));

    CoverageComponent = CreateDefaultSubobject<UMACoverageComponent>(TEXT("CoverageComponent"));
    CoverageComponent->ScanRadius = 200.f;
    
    // ========== StateTree 组件 ==========
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(true);
    
    // ========== Mesh 设置 ==========
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
        TEXT("/Game/Robot/go1/go1.go1"));
    if (MeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
        GetMesh()->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
    }

    // ========== 动画设置 ==========
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
    
    // ========== 移动设置 ==========
    GetCharacterMovement()->MaxWalkSpeed = 150.f;
    GetCharacterMovement()->MaxAcceleration = 512.f;
    GetCharacterMovement()->JumpZVelocity = 300.f;
    GetCharacterMovement()->AirControl = 0.2f;
    GetCharacterMovement()->NavAgentProps.bCanJump = true;
}

void AMARobotDogCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    // 绑定能量耗尽回调
    if (EnergyComponent)
    {
        EnergyComponent->OnEnergyDepleted.AddDynamic(this, &AMARobotDogCharacter::OnEnergyDepleted);
    }
    
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
    
    // 移动时消耗能量 (委托给 EnergyComponent)
    if (bIsMoving && EnergyComponent && EnergyComponent->HasEnergy())
    {
        EnergyComponent->DrainEnergy(DeltaTime);
    }
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

// ========== 便捷访问方法 ==========

float AMARobotDogCharacter::GetEnergy() const
{
    return EnergyComponent ? EnergyComponent->GetEnergy() : 0.f;
}

bool AMARobotDogCharacter::HasEnergy() const
{
    return EnergyComponent ? EnergyComponent->HasEnergy() : false;
}

// ========== 能量耗尽回调 ==========

void AMARobotDogCharacter::OnEnergyDepleted()
{
    CancelNavigation();
    StopFollowing();
    UE_LOG(LogTemp, Warning, TEXT("[%s] Energy depleted! Stopping movement."), *AgentName);
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
