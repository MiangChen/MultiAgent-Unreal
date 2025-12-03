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

    // 设置胶囊体大小
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // 不让控制器旋转角色
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 配置角色移动
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    
    // 启用 RVO 避障
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
    
    // 使用 AIController 的 NavMesh 导航
    if (AAIController* AICtrl = Cast<AAIController>(GetController()))
    {
        AICtrl->MoveToLocation(Destination);
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
