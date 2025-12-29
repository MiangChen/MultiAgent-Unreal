// MAHumanoidCharacter.cpp
// 人形机器人实现

#include "MAHumanoidCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "../StateTree/MAStateTreeComponent.h"
#include "StateTree.h"

AMAHumanoidCharacter::AMAHumanoidCharacter()
{
    AgentType = EMAAgentType::Humanoid;
    bIsPlayingWalk = false;
    
    // StateTree 组件
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(true);
    
    // 加载 Humanoid 模型
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
    if (MeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    }
    
    // 加载动画序列
    static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimAsset(
        TEXT("/Game/Characters/Mannequins/Animations/Manny/MM_Idle.MM_Idle"));
    if (IdleAnimAsset.Succeeded())
    {
        IdleAnim = IdleAnimAsset.Object;
    }
    
    static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAnimAsset(
        TEXT("/Game/Characters/Mannequins/Animations/Manny/MM_Walk_Fwd.MM_Walk_Fwd"));
    if (WalkAnimAsset.Succeeded())
    {
        WalkAnim = WalkAnimAsset.Object;
    }
    
    if (IdleAnim)
    {
        GetMesh()->SetAnimation(IdleAnim);
        GetMesh()->Play(true);
    }
    
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->MaxWalkSpeed = 300.f;
}

void AMAHumanoidCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    // 加载 StateTree 资产（仅当 StateTree 启用时）
    if (StateTreeComponent && StateTreeComponent->IsStateTreeEnabled() && !StateTreeComponent->HasStateTree())
    {
        UStateTree* ST = LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_Humanoid.ST_Humanoid"));
        if (ST)
        {
            StateTreeComponent->SetStateTreeAsset(ST);
            UE_LOG(LogTemp, Log, TEXT("[Humanoid %s] StateTree loaded: ST_Humanoid"), *AgentID);
        }
    }
    else if (StateTreeComponent && !StateTreeComponent->IsStateTreeEnabled())
    {
        UE_LOG(LogTemp, Log, TEXT("[Humanoid %s] StateTree disabled, using direct skill activation"), *AgentID);
    }
}

void AMAHumanoidCharacter::InitializeSkillSet()
{
    // Humanoid 技能: Navigate, Place
    AvailableSkills.Add(EMASkillType::Navigate);
    AvailableSkills.Add(EMASkillType::Place);
    AvailableSkills.Add(EMASkillType::Charge);
}

void AMAHumanoidCharacter::OnNavigationTick()
{
    FVector Velocity = GetCharacterMovement()->Velocity;
    float Speed = Velocity.Size2D();
    if (Speed > 3.0f)
    {
        FVector AccelDir = Velocity.GetSafeNormal2D();
        GetCharacterMovement()->AddInputVector(AccelDir);
    }
}

void AMAHumanoidCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    float Speed = GetVelocity().Size();
    if (Speed > 10.f && !bIsPlayingWalk)
    {
        if (WalkAnim)
        {
            GetMesh()->SetAnimation(WalkAnim);
            GetMesh()->Play(true);
            bIsPlayingWalk = true;
        }
    }
    else if (Speed <= 10.f && bIsPlayingWalk)
    {
        if (IdleAnim)
        {
            GetMesh()->SetAnimation(IdleAnim);
            GetMesh()->Play(true);
            bIsPlayingWalk = false;
        }
    }
}
