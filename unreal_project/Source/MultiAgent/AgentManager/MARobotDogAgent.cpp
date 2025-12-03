#include "MARobotDogAgent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AMARobotDogAgent::AMARobotDogAgent()
{
    AgentType = EAgentType::RobotDog;
    bIsPlayingWalk = false;
    
    // 机器狗的胶囊体更小
    GetCapsuleComponent()->InitCapsuleSize(30.f, 40.0f);
    
    // 设置机器狗 Mesh
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
        TEXT("/Game/Robot/go1/go1.go1"));
    if (MeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
        GetMesh()->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
    }

    // 加载待机动画
    static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimAsset(
        TEXT("/Game/Robot/go1/1Idle.1Idle"));
    if (IdleAnimAsset.Succeeded())
    {
        IdleAnim = IdleAnimAsset.Object;
    }
    
    // 加载行走动画
    static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAnimAsset(
        TEXT("/Game/Robot/go1/1LYP.1LYP"));
    if (WalkAnimAsset.Succeeded())
    {
        WalkAnim = WalkAnimAsset.Object;
    }

    // 默认播放待机动画
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    if (IdleAnim)
    {
        GetMesh()->SetAnimation(IdleAnim);
        GetMesh()->Play(true);
    }
    
    // 移动速度
    GetCharacterMovement()->MaxWalkSpeed = 150.f;
    GetCharacterMovement()->MaxAcceleration = 512.f;
}

void AMARobotDogAgent::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 根据速度切换动画
    float Speed = GetVelocity().Size();
    
    if (Speed > 10.f && !bIsPlayingWalk)
    {
        PlayWalkAnimation();
    }
    else if (Speed <= 10.f && bIsPlayingWalk)
    {
        PlayIdleAnimation();
    }
}

void AMARobotDogAgent::PlayWalkAnimation()
{
    if (WalkAnim)
    {
        GetMesh()->SetAnimation(WalkAnim);
        GetMesh()->SetPlayRate(2.5f);  // 动画播放速度2.5倍
        GetMesh()->Play(true);
        bIsPlayingWalk = true;
    }
}

void AMARobotDogAgent::PlayIdleAnimation()
{
    if (IdleAnim)
    {
        GetMesh()->SetAnimation(IdleAnim);
        GetMesh()->Play(true);
        bIsPlayingWalk = false;
    }
}
