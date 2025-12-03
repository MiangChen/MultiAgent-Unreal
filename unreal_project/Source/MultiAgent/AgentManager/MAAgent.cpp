#include "MAAgent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"

AMAAgent::AMAAgent()
{
    PrimaryActorTick.bCanEverTick = true;
    
    AgentID = 0;
    AgentName = TEXT("Agent");
    AgentType = EAgentType::Human;
    bIsMoving = false;
    TargetLocation = FVector::ZeroVector;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // AIController 配置 - NavMesh 导航必需
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // 角色旋转配置
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 移动组件配置
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    
    // RVO 避障
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->AvoidanceConsiderationRadius = 200.f;
}

void AMAAgent::BeginPlay()
{
    Super::BeginPlay();
}

void AMAAgent::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 为动画蓝图提供加速度输入
    UpdateAnimation();
    
    // 检查是否到达目标
    if (bIsMoving)
    {
        float Distance = FVector::Dist2D(GetActorLocation(), TargetLocation);
        if (Distance < 50.f)
        {
            bIsMoving = false;
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
    // ABP_Manny 需要 Acceleration > 0 才会播放移动动画
    // AIController::MoveToLocation 直接设置 Velocity，不设置 Acceleration
    // 因此需要手动添加输入向量来触发动画
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
}

FVector AMAAgent::GetCurrentLocation() const
{
    return GetActorLocation();
}
