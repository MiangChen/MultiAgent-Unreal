// MAQuadrupedCharacter.cpp

#include "MAQuadrupedCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "../../StateTree/Runtime/MAStateTreeComponent.h"
#include "StateTree.h"

AMAQuadrupedCharacter::AMAQuadrupedCharacter()
{
    AgentType = EMAAgentType::Quadruped;
    bIsPlayingWalk = false;
    
    // StateTree 组件
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(false);
    
    // Mesh 设置
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
        TEXT("/Game/Robot/go1/go1.go1"));
    if (MeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
    }

    // 动画设置
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
    
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
}

void AMAQuadrupedCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (SkillComponent)
    {
        SkillComponent->OnEnergyDepleted.AddDynamic(this, &AMAQuadrupedCharacter::OnEnergyDepleted);
    }
    
    // 加载 StateTree 资产（仅当 StateTree 启用时）
    if (StateTreeComponent && StateTreeComponent->IsStateTreeEnabled() && !StateTreeComponent->HasStateTree())
    {
        UStateTree* ST = LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_Quadruped.ST_Quadruped"));
        if (ST)
        {
            StateTreeComponent->SetStateTreeAsset(ST);
            UE_LOG(LogTemp, Log, TEXT("[Quadruped %s] StateTree loaded: ST_Quadruped"), *AgentID);
        }
    }
    else if (StateTreeComponent && !StateTreeComponent->IsStateTreeEnabled())
    {
        UE_LOG(LogTemp, Log, TEXT("[Quadruped %s] StateTree disabled, using direct skill activation"), *AgentID);
    }
}

void AMAQuadrupedCharacter::InitializeSkillSet()
{
    // Quadruped 技能: Navigate, Search, Follow
    AvailableSkills.Add(EMASkillType::Navigate);
    AvailableSkills.Add(EMASkillType::Search);
    AvailableSkills.Add(EMASkillType::Follow);
    AvailableSkills.Add(EMASkillType::Charge);
}

void AMAQuadrupedCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    float Speed = GetVelocity().Size();
    if (Speed > 10.f && !bIsPlayingWalk)
    {
        PlayWalkAnimation();
    }
    else if (Speed <= 10.f && bIsPlayingWalk)
    {
        PlayIdleAnimation();
    }
    
    if (bIsMoving && ShouldDrainEnergy() && SkillComponent && SkillComponent->HasEnergy())
    {
        SkillComponent->DrainEnergy(DeltaTime);
    }
}

void AMAQuadrupedCharacter::PlayWalkAnimation()
{
    if (WalkAnim)
    {
        GetMesh()->SetAnimation(WalkAnim);
        GetMesh()->SetPlayRate(2.5f);
        GetMesh()->Play(true);
        bIsPlayingWalk = true;
    }
}

void AMAQuadrupedCharacter::PlayIdleAnimation()
{
    if (IdleAnim)
    {
        GetMesh()->SetAnimation(IdleAnim);
        GetMesh()->Play(true);
        bIsPlayingWalk = false;
    }
}

void AMAQuadrupedCharacter::OnEnergyDepleted()
{
    CancelNavigation();
    StopFollowing();
}
