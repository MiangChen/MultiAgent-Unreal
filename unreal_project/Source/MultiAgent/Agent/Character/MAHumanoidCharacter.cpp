// MAHumanoidCharacter.cpp
// 人形机器人实现

#include "MAHumanoidCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "../StateTree/MAStateTreeComponent.h"
#include "StateTree.h"
#include "TimerManager.h"

AMAHumanoidCharacter::AMAHumanoidCharacter()
{
    AgentType = EMAAgentType::Humanoid;
    bIsPlayingWalk = false;
    bIsBending = false;
    
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
    
    // 加载基础动画序列
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
    
    // 加载俯身/起身动画序列
    // 使用 MM_Land 作为俯身动画（蹲下姿势）
    // 使用 MM_Jump 作为起身动画（跳起/站立姿势）
    static ConstructorHelpers::FObjectFinder<UAnimSequence> BendDownAnimAsset(
        TEXT("/Game/Characters/Mannequins/Animations/Manny/MM_Land.MM_Land"));
    if (BendDownAnimAsset.Succeeded())
    {
        BendDownAnim = BendDownAnimAsset.Object;
        UE_LOG(LogTemp, Log, TEXT("[Humanoid] BendDown animation loaded (MM_Land)"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Humanoid] BendDown animation (MM_Land) not found, will use timer fallback"));
    }
    
    static ConstructorHelpers::FObjectFinder<UAnimSequence> StandUpAnimAsset(
        TEXT("/Game/Characters/Mannequins/Animations/Manny/MM_Jump.MM_Jump"));
    if (StandUpAnimAsset.Succeeded())
    {
        StandUpAnim = StandUpAnimAsset.Object;
        UE_LOG(LogTemp, Log, TEXT("[Humanoid] StandUp animation loaded (MM_Jump)"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Humanoid] StandUp animation (MM_Jump) not found, will use timer fallback"));
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
    
    // 如果正在播放俯身/起身动画，不切换到行走/待机动画
    if (bIsBending)
    {
        return;
    }
    
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

void AMAHumanoidCharacter::PlayBendDownAnimation()
{
    if (bIsBending)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Humanoid %s] Already playing bend animation"), *AgentID);
        return;
    }
    
    bIsBending = true;
    bIsPlayingWalk = false;
    
    UE_LOG(LogTemp, Log, TEXT("[Humanoid %s] Playing bend down animation"), *AgentID);
    
    // 播放俯身动画（如果存在）
    if (BendDownAnim)
    {
        GetMesh()->SetAnimation(BendDownAnim);
        GetMesh()->Play(false); // 不循环
    }
    
    // 设置定时器，动画完成后回调
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            BendAnimTimerHandle,
            this,
            &AMAHumanoidCharacter::OnBendAnimFinished,
            BendAnimDuration,
            false
        );
    }
}

void AMAHumanoidCharacter::PlayStandUpAnimation()
{
    if (bIsBending)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Humanoid %s] Already playing bend animation"), *AgentID);
        return;
    }
    
    bIsBending = true;
    bIsPlayingWalk = false;
    
    UE_LOG(LogTemp, Log, TEXT("[Humanoid %s] Playing stand up animation"), *AgentID);
    
    // 播放起身动画（如果存在）
    if (StandUpAnim)
    {
        GetMesh()->SetAnimation(StandUpAnim);
        GetMesh()->Play(false); // 不循环
    }
    
    // 设置定时器，动画完成后回调
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            BendAnimTimerHandle,
            this,
            &AMAHumanoidCharacter::OnBendAnimFinished,
            BendAnimDuration,
            false
        );
    }
}

void AMAHumanoidCharacter::OnBendAnimFinished()
{
    UE_LOG(LogTemp, Log, TEXT("[Humanoid %s] Bend animation finished"), *AgentID);
    
    bIsBending = false;
    
    // 恢复到待机动画
    if (IdleAnim)
    {
        GetMesh()->SetAnimation(IdleAnim);
        GetMesh()->Play(true);
    }
    
    // 广播动画完成事件
    OnBendAnimationComplete.Broadcast();
}
