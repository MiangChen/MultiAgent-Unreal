// MAHumanoidCharacter.cpp
// 人形机器人实现

#include "MAHumanoidCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "../../StateTree/Runtime/MAStateTreeComponent.h"
#include "StateTree.h"
#include "TimerManager.h"

AMAHumanoidCharacter::AMAHumanoidCharacter()
{
    AgentType = EMAAgentType::Humanoid;
    bIsPlayingWalk = false;
    bIsBending = false;
    
    // StateTree 组件
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(false);
    
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
    
    // 加载拾取动画（Item Pickup Set）
    // 动画结构: 站立(0%) → 弯腰(~50%) → 站立(100%)
    // 我们将其分为两半使用：
    //   - BendDown: 播放 0% → 50%（站立→弯腰）
    //   - StandUp:  播放 50% → 100%（弯腰→站立）
    static ConstructorHelpers::FObjectFinder<UAnimSequence> PickupAnimAsset(
        TEXT("/Game/ItemPickupSet/Animations/Mannequin/fromIdle/A_ItemPickup_fromIdle_RH_0cm.A_ItemPickup_fromIdle_RH_0cm"));
    if (PickupAnimAsset.Succeeded())
    {
        PickupAnim = PickupAnimAsset.Object;
        
        // 获取动画实际时长
        PickupAnimDuration = PickupAnim->GetPlayLength();
        // 动画中点（弯腰最低点）大约在 45-50% 处
        PickupAnimMidPoint = PickupAnimDuration * 0.5f;
        
        UE_LOG(LogTemp, Log, TEXT("[Humanoid] Pickup animation loaded, Duration: %.2fs, MidPoint: %.2fs"), 
            PickupAnimDuration, PickupAnimMidPoint);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Humanoid] Pickup animation not found!"));
    }
    
    if (IdleAnim)
    {
        GetMesh()->SetAnimation(IdleAnim);
        GetMesh()->Play(true);
    }
    
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
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
        UE_LOG(LogTemp, Warning, TEXT("[Humanoid %s] Already playing animation"), *AgentID);
        return;
    }
    
    bIsBending = true;
    bIsPlayingWalk = false;
    
    UE_LOG(LogTemp, Log, TEXT("[Humanoid %s] Playing bend down animation (0%% -> 50%%)"), *AgentID);
    
    if (PickupAnim)
    {
        // 播放动画前半段：从开始(0)到中点(弯腰最低点)
        GetMesh()->SetAnimation(PickupAnim);
        GetMesh()->SetPlayRate(1.0f);
        GetMesh()->SetPosition(0.0f);  // 从头开始
        GetMesh()->Play(false);        // 不循环
        
        // 定时器：在中点停止
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                BendAnimTimerHandle,
                this,
                &AMAHumanoidCharacter::OnBendDownAnimFinished,
                PickupAnimMidPoint,
                false
            );
        }
    }
    else
    {
        // 没有动画，直接完成
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                BendAnimTimerHandle,
                this,
                &AMAHumanoidCharacter::OnBendDownAnimFinished,
                0.5f,
                false
            );
        }
    }
}

void AMAHumanoidCharacter::OnBendDownAnimFinished()
{
    UE_LOG(LogTemp, Log, TEXT("[Humanoid %s] Bend down animation finished (at midpoint)"), *AgentID);
    
    // 停止动画播放，保持在弯腰姿势
    if (PickupAnim)
    {
        GetMesh()->Stop();
        GetMesh()->SetPosition(PickupAnimMidPoint);  // 保持在中点位置
    }
    
    bIsBending = false;
    
    // 广播动画完成事件
    OnBendAnimationComplete.Broadcast();
}

void AMAHumanoidCharacter::PlayStandUpAnimation()
{
    if (bIsBending)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Humanoid %s] Already playing animation"), *AgentID);
        return;
    }
    
    bIsBending = true;
    bIsPlayingWalk = false;
    
    UE_LOG(LogTemp, Log, TEXT("[Humanoid %s] Playing stand up animation (50%% -> 100%%)"), *AgentID);
    
    if (PickupAnim)
    {
        // 播放动画后半段：从中点(弯腰)到结束(站立)
        GetMesh()->SetAnimation(PickupAnim);
        GetMesh()->SetPlayRate(1.0f);
        GetMesh()->SetPosition(PickupAnimMidPoint);  // 从中点开始
        GetMesh()->Play(false);
        
        // 定时器：播放后半段时长
        float StandUpDuration = PickupAnimDuration - PickupAnimMidPoint;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                BendAnimTimerHandle,
                this,
                &AMAHumanoidCharacter::OnStandUpAnimFinished,
                StandUpDuration,
                false
            );
        }
    }
    else
    {
        // 没有动画，直接完成
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                BendAnimTimerHandle,
                this,
                &AMAHumanoidCharacter::OnStandUpAnimFinished,
                0.5f,
                false
            );
        }
    }
}

void AMAHumanoidCharacter::OnStandUpAnimFinished()
{
    UE_LOG(LogTemp, Log, TEXT("[Humanoid %s] Stand up animation finished"), *AgentID);
    
    bIsBending = false;
    
    // 恢复到待机动画
    if (IdleAnim)
    {
        GetMesh()->SetAnimation(IdleAnim);
        GetMesh()->SetPlayRate(1.0f);
        GetMesh()->Play(true);
    }
    
    // 广播动画完成事件
    OnBendAnimationComplete.Broadcast();
}
